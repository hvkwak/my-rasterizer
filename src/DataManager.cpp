#include "DataManager.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Point.h"
#include "Utils.h"
#include "Cache.h"
#include "Job.h"
#include "Block.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

// Constructor
DataManager::DataManager() {}

bool DataManager::init(const std::filesystem::path& plyPath,
                       const std::filesystem::path& outDir_,
                       bool isOOC_,
                       glm::vec3& bb_min_,
                       glm::vec3& bb_max_,
                       std::vector<Block>& blocks)
{
  // Store outDir
  outDir = outDir_;

  // Clean up old block files (rm ../data/*.bin)
  if (std::filesystem::exists(outDir)) {
    for (const auto &entry : std::filesystem::directory_iterator(outDir)) {
      if (entry.path().extension() == ".bin") {
        std::filesystem::remove(entry.path());
      }
    }
    std::cout << "Cleaned up old block files in " << outDir << std::endl;
  }

  // reads .ply
  if (!readPLY(plyPath, bb_min_, bb_max_)){
    std::cerr << "Error: Failed to do readPLY()" <<  std::endl;
    return false;
  }

  // sets up LRU cache for file writes when creating block files
  if (!cache.init(CACHE_SIZE)) {
    std::cerr << "Error: Failed to initialize cache." << std::endl;
    return false;
  }

  // create Blocks
  if (!createBlocks(plyPath, bb_min_, bb_max_, blocks, isOOC_)) {
    std::cerr << "Error: Failed to do createBlocks()" << std::endl;
    return false;
  }

  // Out-of-core and in-core
  if (isOOC_){
    // setup multi-threading workers for out-of-core load
    workers.clear();
    workers.reserve(NUM_WORKERS);
    for (int i = 0; i < NUM_WORKERS; i++){
      workers.emplace_back(workerMain, i, std::ref(jobQ), std::ref(resultQ));
    }
  } else {
    // all block points in-core.
    for (int id = 0; id < NUM_BLOCKS; id++){
      loadBlock(pathFor(id), blocks[id].count, blocks[id].points);
    }
  }

  return true;
}

/**
 * @brief reads a .ply file and compute a global bbox
 *
 */
bool DataManager::readPLY(const std::filesystem::path& plyPath, glm::vec3& bb_min_, glm::vec3& bb_max_)
{
  std::ifstream file(plyPath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open PLY file: " << plyPath << std::endl;
    return false;
  }

  // parse header
  std::string line, word, format;
  bool headerEnded = false;

  while (std::getline(file, line)) {
    std::stringstream ss(line);
    ss >> word;
    if (word == "format") {
      ss >> format;
    } else if (word == "element") {
      std::string what;
      ss >> what;
      if (what == "vertex")
        ss >> vertexCount;
    } else if (word == "end_header") {
      headerEnded = true;
      break;
    }
  }

  if (!headerEnded || vertexCount == 0) {
    std::cerr << "Error: Invalid PLY header in file: " << plyPath << std::endl;
    return false;
  }
  if (format != "binary_little_endian") {
    std::cerr << "Only binary_little_endian supported in this fast path.\n";
    return false;
  }

  // IMPORTANT: remember where binary vertex data begins
  dataStart = file.tellg();

  // compute global bbox
  std::vector<FilePoint> buf(BATCH);

  file.seekg(dataStart);
  uint64_t remaining = vertexCount;
  while (remaining > 0) {
    uint64_t take = std::min<uint64_t>(remaining, buf.size());
    file.read(reinterpret_cast<char *>(buf.data()), (std::streamsize)(take * sizeof(FilePoint)));
    if ((uint64_t)file.gcount() != take * sizeof(FilePoint)) {
      std::cerr << "Error: Failed to read vertex data (bbox pass) from: " << plyPath << std::endl;
      return false;
    }

    for (uint64_t i = 0; i < take; ++i) {
      glm::vec3 p((float)buf[i].x, (float)buf[i].y, (float)buf[i].z);
      bboxExpand(p, bb_min_, bb_max_);
    }
    remaining -= take;
  }

  return true;
}

bool DataManager::createBlocks(const std::filesystem::path& plyPath, const glm::vec3& bb_min_, const glm::vec3& bb_max_, std::vector<Block>& blocks, bool isOOC_)
{
  std::ifstream file(plyPath, std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error: Could not open PLY file: " << plyPath << std::endl;
    return false;
  }

  // block size
  glm::vec3 extent = bb_max_ - bb_min_;
  glm::vec3 cell = extent / (float)GRID;

  // block bbox update
  for (int z = 0; z < GRID; ++z) {
    for (int y = 0; y < GRID; ++y) {
      for (int x = 0; x < GRID; ++x) {
        int id = x + GRID * y + GRID * GRID * z;

        glm::vec3 mn = bb_min_ + glm::vec3(x, y, z) * cell;
        glm::vec3 mx = bb_min_ + glm::vec3(x + 1, y + 1, z + 1) * cell;

        blocks[id].bb_min = mn;
        blocks[id].bb_max = mx;
      }
    }
  }
  // write block files with LRU Cache
  for (int id = 0; id < NUM_BLOCKS; ++id) {
    cache.get(id, pathFor(id));
  }

  std::vector<FilePoint> buf(BATCH);
  std::array<std::vector<Point>, NUM_BLOCKS> outBuf;
  for (auto &v : outBuf)
    v.reserve(FLUSH_POINTS);

  file.clear();
  file.seekg(dataStart);

  uint64_t remaining = vertexCount;
  while (remaining > 0) {
    uint64_t take = std::min<uint64_t>(remaining, buf.size());
    file.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(take * sizeof(FilePoint)));
    if ((uint64_t)file.gcount() != take * sizeof(FilePoint)) {
      std::cerr << "Error: Failed to read vertex data (block creation pass) from: " << plyPath << std::endl;
      return false;
    }

    for (uint64_t i = 0; i < take; ++i) {
      const auto &fp = buf[i];
      float x = (float)fp.x;
      float y = (float)fp.y;
      float z = (float)fp.z;

      int ix = clampi((int)((x - bb_min_.x) / cell.x), 0, 9);
      int iy = clampi((int)((y - bb_min_.y) / cell.y), 0, 9);
      int iz = clampi((int)((z - bb_min_.z) / cell.z), 0, 9);
      int id = ix + 10 * iy + 100 * iz;

      outBuf[id].push_back(Point{glm::vec3(x, y, z), glm::vec3(fp.r/255.0f, fp.g/255.0f, fp.b/255.0f)});
      blocks[id].count++;

      if (outBuf[id].size() >= FLUSH_POINTS)
        flush(id, outBuf);
    }
    remaining -= take;
  }

  // flush leftovers
  for (int id = 0; id < NUM_BLOCKS; ++id){
    flush(id, outBuf);
  }

  cache.close_all();
  std::cout << "created " << NUM_BLOCKS << "blocks." << std::endl;
  return true;
}

void DataManager::enqueueBlock(int id, int count) {
  Job job;
  job.blockID = id;
  job.count = count;
  job.path = pathFor(id);
  jobQ.push(std::move(job));
}

std::filesystem::path DataManager::pathFor(int id) {
  char name[64];
  std::snprintf(name, sizeof(name), "block_%04d.bin", id);
  return outDir / name;
}

void DataManager::loadBlock(const std::filesystem::path& path, const int & count, Result & r){
  std::ifstream is(path, std::ios::binary);

  /*ADDED ERROR HANDLING*/
  if (!is.is_open()) {
    std::cerr << "Error: Could not open block file: " << path << std::endl;
    r.points.clear();
    return;
  }

  r.points.resize(count);
  is.read(reinterpret_cast<char *>(r.points.data()), count * sizeof(Point));

  /*ADDED ERROR HANDLING*/
  if (!is.good() && !is.eof()) {
    std::cerr << "Error: Failed to read block data from: " << path << std::endl;
    r.points.clear();
    return;
  }

  /*ADDED ERROR HANDLING*/
  if ((size_t)is.gcount() != count * sizeof(Point)) {
    std::cerr << "Error: Incomplete read from block file: " << path
              << " (expected " << count * sizeof(Point)
              << " bytes, got " << is.gcount() << " bytes)" << std::endl;
    r.points.clear();
    return;
  }
}

void DataManager::loadBlock(const std::filesystem::path& path, const int & count, std::vector<Point>& points){
  std::ifstream is(path, std::ios::binary);

  /*ADDED ERROR HANDLING*/
  if (!is.is_open()) {
    std::cerr << "Error: Could not open block file: " << path << std::endl;
    points.clear();
    return;
  }

  points.resize(count);
  is.read(reinterpret_cast<char *>(points.data()), count * sizeof(Point));

  /*ADDED ERROR HANDLING*/
  if (!is.good() && !is.eof()) {
    std::cerr << "Error: Failed to read block data from: " << path << std::endl;
    points.clear();
    return;
  }

  /*ADDED ERROR HANDLING*/
  if ((size_t)is.gcount() != count * sizeof(Point)) {
    std::cerr << "Error: Incomplete read from block file: " << path
              << " (expected " << count * sizeof(Point)
              << " bytes, got " << is.gcount() << " bytes)" << std::endl;
    points.clear();
    return;
  }
}

void DataManager::workerMain(int workerID, Queue<Job>& jobQ, Queue<Result>& resultQ)
{
  Job job;
  while (jobQ.pop(job)) {

    // Read binary file
    Result r;
    r.blockID = job.blockID;
    loadBlock(job.path, job.count, r);

    // Read binary file
    // std::ifstream is(job.path, std::ios::binary);
    // r.points.resize(count);
    // is.read(reinterpret_cast<char*>(r.points.data()), job.count * sizeof(Point));

    // move to Result
    resultQ.push(std::move(r));
  }
  // jobQ.stop() called -> threads are over
}

void DataManager::getResult(Result& out) {
  resultQ.pop(out);
}

void DataManager::quit(){
  jobQ.stop();
  for (auto& t : workers) t.join();
}

void DataManager::bboxExpand(const glm::vec3& p, glm::vec3& bb_min_, glm::vec3& bb_max_) {
  if (p.x < bb_min_.x)
    bb_min_.x = p.x;
  if (p.y < bb_min_.y)
    bb_min_.y = p.y;
  if (p.z < bb_min_.z)
    bb_min_.z = p.z;
  if (p.x > bb_max_.x)
    bb_max_.x = p.x;
  if (p.y > bb_max_.y)
    bb_max_.y = p.y;
  if (p.z > bb_max_.z)
    bb_max_.z = p.z;
}

// Private helper: flush buffer to disk
void DataManager::flush(int id, std::array<std::vector<Point>, NUM_BLOCKS>& outBuf) {
  auto &v = outBuf[id];
  if (v.empty())
    return;
  auto &os = cache.get(id, pathFor(id));
  os.write(reinterpret_cast<const char *>(v.data()), (std::streamsize)(v.size() * sizeof(Point)));

  /*ADDED ERROR HANDLING*/
  if (!os.good()) {
    std::cerr << "Error: Failed to write block data to: " << pathFor(id)
              << " (block " << id << ", " << v.size() << " points)" << std::endl;
  }

  v.clear();
}

// Previous readPLY
// /**
//  * @brief reads .ply binary point cloud data
//  *
//  * @param filename filename of .ply file
//  * @return vector of 3D points
//  */
// std::vector<Point> DataManager::readPLY(const std::string &filename,
//                                         glm::vec3 &bb_min_,
//                                         glm::vec3 &bb_max_)
// {

//   std::ifstream file(filename, std::ios::binary);
//   std::vector<Point> finalPoints;

//   if (!file.is_open()) {
//     std::cerr << "Error: Could not open file " << filename << std::endl;
//     return finalPoints;
//   }

//   // parse header
//   std::string line;
//   long vertexCount = 0;
//   bool headerEnded = false;

//   while (std::getline(file, line)) {
//     std::stringstream ss(line);
//     std::string word;
//     ss >> word;

//     if (word == "element") {
//       ss >> word;
//       if (word == "vertex")
//         ss >> vertexCount;
//     } else if (word == "end_header") {
//       headerEnded = true;
//       break;
//     }
//   }

//   if (!headerEnded || vertexCount == 0) {
//     std::cerr << "Error: Invalid PLY header in file: " << filename << std::endl;
//     return finalPoints;
//   }

//   // read binary data
//   // 1. Create a buffer to hold the raw file data
//   std::vector<FilePoint> rawData(vertexCount);

//   // 2. Read the file into the buffer in one go
//   file.read(reinterpret_cast<char *>(rawData.data()),
//             vertexCount * sizeof(FilePoint));

//   if (file.gcount() != vertexCount * sizeof(FilePoint)) {
//     std::cerr << "Error: Corrupted file or unexpected end of file."
//               << std::endl;
//     finalPoints.clear();
//     return finalPoints;
//   }

//   // convert to GLM format
//   // Reserve memory for your actual points
//   finalPoints.resize(vertexCount);

//   for (size_t i = 0; i < vertexCount; ++i) {
//     // Convert Double (File) -> Float (GLM)
//     FilePoint point = rawData[i];
//     float x = static_cast<float>(point.x);
//     float y = static_cast<float>(point.y);
//     float z = static_cast<float>(point.z);
//     finalPoints[i].pos = glm::vec3(x, y, z);

//     // update bounding box
//     bboxExpand(finalPoints[i].pos, bb_min_, bb_max_);

//     // Convert Uchar [0-255] -> Float [0.0-1.0] (GLM Standard)
//     finalPoints[i].color = glm::vec3(static_cast<float>(point.r) / 255.0f,
//                                      static_cast<float>(point.g) / 255.0f,
//                                      static_cast<float>(point.b) / 255.0f);
//   }

//   std::cout << "Successfully converted " << finalPoints.size() << " points."
//             << std::endl;
//   return finalPoints;
// }
