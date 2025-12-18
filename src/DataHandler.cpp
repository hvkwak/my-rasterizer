#include "DataHandler.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Point.h"
#include "Utils.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

// Constructor
DataHandler::DataHandler() {}

// Getters
// const glm::vec3& DataHandler::getBBoxMin() const {
//     return bb_min_;
// }

// const glm::vec3& DataHandler::getBBoxMax() const {
//     return bb_max_;
// }



// Private helper: expand bounding box
void DataHandler::bboxExpand(const glm::vec3& p, glm::vec3& bb_min_, glm::vec3& bb_max_) {
    if (p.x < bb_min_.x) bb_min_.x = p.x;
    if (p.y < bb_min_.y) bb_min_.y = p.y;
    if (p.z < bb_min_.z) bb_min_.z = p.z;
    if (p.x > bb_max_.x) bb_max_.x = p.x;
    if (p.y > bb_max_.y) bb_max_.y = p.y;
    if (p.z > bb_max_.z) bb_max_.z = p.z;
}

// Private helper: flush buffer to disk
void DataHandler::flush(int id, std::vector<std::ofstream>& outs, std::array<std::vector<PointDisk>, BLOCKS>& outBuf) {
    auto& v = outBuf[id];
    if (v.empty()) return;
    outs[id].write(reinterpret_cast<const char*>(v.data()), static_cast<std::streamsize>(v.size() * sizeof(PointDisk)));
    v.clear(); // keeps capacity -> no reallocation next time
}

/**
 * @brief reads a .ply file and divide points into 10x10x10 seperate blocks (.bin files).
 */
bool DataHandler::createBlocks(const std::string& plyPath, const std::string& outDir, glm::vec3& bb_min_, glm::vec3& bb_max_) {

    std::ifstream file(plyPath, std::ios::binary);
    if (!file.is_open()) return false;

    // parse header
    std::string line, word, format;
    uint64_t vertexCount = 0;
    bool headerEnded = false;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        ss >> word;
        if (word == "format") {
            ss >> format; // binary_little_endian
        } else if (word == "element") {
            std::string what;
            ss >> what;
            if (what == "vertex") ss >> vertexCount;
        } else if (word == "end_header") {
            headerEnded = true;
            break;
        }
    }
    if (!headerEnded || vertexCount == 0) return false;
    if (format != "binary_little_endian") {
        std::cerr << "Only binary_little_endian supported in this fast path.\n";
        return false;
    }

    // IMPORTANT: remember where binary vertex data begins
    const std::streampos dataStart = file.tellg();

    // PART 1: compute global bbox
    const size_t BATCH = 1u << 16; // e.g. 65536.
    std::vector<FilePoint> buf(BATCH);

    file.seekg(dataStart);
    uint64_t remaining = vertexCount;
    while (remaining > 0) {
        uint64_t take = std::min<uint64_t>(remaining, buf.size());
        file.read(reinterpret_cast<char*>(buf.data()), (std::streamsize)(take * sizeof(FilePoint)));
        if ((uint64_t)file.gcount() != take * sizeof(FilePoint)) return false;

        for (uint64_t i = 0; i < take; ++i) {
            glm::vec3 p((float)buf[i].x, (float)buf[i].y, (float)buf[i].z);
            bboxExpand(p, bb_min_, bb_max_);
        }
        remaining -= take;
    }

    // voxel size
    glm::vec3 extent = bb_max_ - bb_min_;
    glm::vec3 cell = extent / 10.0f;
    cell.x = 1.0f;
    cell.y = 1.0f;
    cell.z = 1.0f;

    std::filesystem::create_directories(outDir);

    // PART 2: stream again and write to voxel block files
    // NOTE: opening BLOCKS files at once may hit your OS fd limit (often 1024).
    // If that happens, switch to an on-demand open/close cache (I can give that too).
    std::vector<std::ofstream> outs(BLOCKS);
    std::vector<uint32_t> counts(BLOCKS, 0);

    for (int id = 0; id < BLOCKS; ++id) {
        char name[64];
        std::snprintf(name, sizeof(name), "block_%04d.bin", id);
        outs[id].open((std::filesystem::path(outDir) / name).string(), std::ios::binary);

        uint32_t placeholder = 0;
        outs[id].write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder)); // patch later
    }

    std::array<std::vector<PointDisk>, BLOCKS> outBuf;
    for (auto& v : outBuf) v.reserve(FLUSH_POINTS);

    file.clear();
    file.seekg(dataStart);

    remaining = vertexCount;
    while (remaining > 0) {
        uint64_t take = std::min<uint64_t>(remaining, buf.size());
        file.read(reinterpret_cast<char*>(buf.data()),
                  static_cast<std::streamsize>(take * sizeof(FilePoint)));
        if ((uint64_t)file.gcount() != take * sizeof(FilePoint))
            return false;

        for (uint64_t i = 0; i < take; ++i) {
            const auto& fp = buf[i];
            float x = (float)fp.x, y = (float)fp.y, z = (float)fp.z;

            int ix = clampi((int)((x - bb_min_.x) / cell.x), 0, 9);
            int iy = clampi((int)((y - bb_min_.y) / cell.y), 0, 9);
            int iz = clampi((int)((z - bb_min_.z) / cell.z), 0, 9);
            int id = ix + 10 * iy + 100 * iz;

            outBuf[id].push_back(PointDisk{x, y, z, fp.r, fp.g, fp.b});
            counts[id]++;

            if (outBuf[id].size() >= FLUSH_POINTS)
                flush(id, outs, outBuf);
        }

        remaining -= take;
    }

    // flush leftovers
    for (int id = 0; id < BLOCKS; ++id) flush(id, outs, outBuf);

    // patch counts
    for (int id = 0; id < BLOCKS; ++id) {
        outs[id].seekp(0, std::ios::beg);
        outs[id].write(reinterpret_cast<const char*>(&counts[id]), sizeof(uint32_t));
        outs[id].close();
    }

    return true;
}


/**
 * @brief reads .ply binary point cloud data
 *
 * @param filename filename of .ply file
 * @return vector of 3D points
 */
std::vector<Point> DataHandler::readPLY(const std::string& filename, glm::vec3& bb_min_, glm::vec3& bb_max_) {

    std::ifstream file(filename, std::ios::binary);
    std::vector<Point> finalPoints;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return finalPoints;
    }

    // parse header
    std::string line;
    long vertexCount = 0;
    bool headerEnded = false;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "element") {
            ss >> word;
            if (word == "vertex") ss >> vertexCount;
        }
        else if (word == "end_header") {
            headerEnded = true;
            break;
        }
    }

    if (!headerEnded || vertexCount == 0) return finalPoints;

    // read binary data
    // 1. Create a buffer to hold the raw file data
    std::vector<FilePoint> rawData(vertexCount);

    // 2. Read the file into the buffer in one go
    file.read(reinterpret_cast<char*>(rawData.data()), vertexCount * sizeof(FilePoint));

    if (file.gcount() != vertexCount * sizeof(FilePoint)) {
        std::cerr << "Error: Corrupted file or unexpected end of file." << std::endl;
        finalPoints.clear();
        return finalPoints;
    }

    // convert to GLM format
    // Reserve memory for your actual points
    finalPoints.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        // Convert Double (File) -> Float (GLM)
        FilePoint point = rawData[i];
        float x = static_cast<float>(point.x);
        float y = static_cast<float>(point.y);
        float z = static_cast<float>(point.z);
        finalPoints[i].pos = glm::vec3(x, y, z);

        // update bounding box
        bboxExpand(finalPoints[i].pos, bb_min_, bb_max_);

        // Convert Uchar [0-255] -> Float [0.0-1.0] (GLM Standard)
        finalPoints[i].color = glm::vec3(
            static_cast<float>(point.r) / 255.0f,
            static_cast<float>(point.g) / 255.0f,
            static_cast<float>(point.b) / 255.0f
        );
    }

    std::cout << "Successfully converted " << finalPoints.size() << " points." << std::endl;
    return finalPoints;
}
