#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <thread>
#include <string>
#include <vector>
#include <array>
#include "Job.h"
#include "Queue.h"
#include "Block.h"
#include "Point.h"
#include "Cache.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr size_t BATCH = 1u << 16; // e.g. 65536.
constexpr size_t CACHE_SIZE = 128;
constexpr size_t FLUSH_POINTS = 4096;
constexpr int NUM_WORKERS = 5;

class DataManager {

public:
  DataManager();

  // sets up DataManager/Cache
  bool init(const std::string & plyPath, const std::string &outDir_, bool isOOC_, glm::vec3& bb_min_, glm::vec3& bb_max_, std::vector<Block>& blocks);

  // load Block
  void loadBlock(unsigned int id, unsigned int count);

  // quit
  void quit();

  // Get result from worker threads (non-blocking)
  void getResult(Result& out);

  // Reset bounding box to initial state
  void resetBBox();

private:

  // num blocks
  unsigned int num_blocks = 0;

  // remember where binary vertex data begins in the .ply file
  std::streampos dataStart;
  uint64_t vertexCount = 0;

  // Cache, Queue and workers
  Cache cache;
  Queue<Job> jobQ;
  Queue<Result> resultQ;
  std::vector<std::thread> workers;
  std::string outDir;

  // load .ply and create blocks
  bool readPLY(const std::string& plyPath, glm::vec3& bb_min_, glm::vec3& bb_max_);
  bool createBlocks(const std::string& plyPath, const glm::vec3& bb_min_, const glm::vec3& bb_max_, std::vector<Block>& blocks);

  void flush(int id, std::array<std::vector<Point>, NUM_BLOCKS> &outBuf);
  void bboxExpand(const glm::vec3 &p, glm::vec3 &bb_min_, glm::vec3 &bb_max);

  static void workerMain(int worker_id, Queue<Job>& jobQ, Queue<Result>& resultQ, const std::string& outDir);
};

#endif // DATAMANAGER_H
