//=============================================================================
//
//   DataManager - Multi-threaded point cloud data loading and management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <thread>
#include <vector>
#include <array>
#include <filesystem>
#include "Job.h"
#include "Queue.h"
#include "Block.h"
#include "Point.h"
#include "FileStreamCache.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr size_t BATCH = 1u << 16; // e.g. 65536.
constexpr size_t CACHE_SIZE = 128;
constexpr size_t FLUSH_POINTS = 4096;
constexpr int NUM_WORKERS = 5;

class DataManager {

public:
  DataManager();

  /** @brief Initialize data manager and load PLY file */
  bool init(const std::filesystem::path& plyPath, const std::filesystem::path& outDir_, bool isOOC_, glm::vec3& bb_min_, glm::vec3& bb_max_, std::vector<Block>& blocks, uint64_t& vertexCount);

  /** @brief Enqueue block loading job */
  void enqueueBlock(const int& blockID, const int& slotIdx, const int& count, const bool& isSub);

  /** @brief Stop worker threads and cleanup */
  void quit();

  /** @brief Get loaded block result from worker threads */
  void getResult(Result& out);

  /** @brief Reset bounding box to initial state */
  void resetBBox();

private:

  // num blocks
  unsigned int num_blocks = 0;

  // remember where binary vertex data begins in the .ply file
  std::streampos dataStart;
  uint64_t vertexCount = 0;

  // FileStreamCache, Queue and workers
  FileStreamCache cache;
  Queue<Job> jobQ;
  Queue<Result> resultQ;
  std::vector<std::thread> workers;
  std::filesystem::path outDir;

  /** @brief Read PLY file and compute global bounding box */
  bool readPLY(const std::filesystem::path& plyPath, glm::vec3& bb_min_, glm::vec3& bb_max_, uint64_t& vertexCount_);

  /** @brief Create spatial blocks from point cloud */
  bool createBlocks(const std::filesystem::path& plyPath, const glm::vec3& bb_min_, const glm::vec3& bb_max_, std::vector<Block>& blocks, bool isOOC_);

  /** @brief Get file path for block ID */
  std::filesystem::path pathFor(int id);

  /** @brief Flush point buffer to disk */
  void flush(int id, std::array<std::vector<Point>, NUM_BLOCKS> &outBuf);

  /** @brief Expand bounding box to include point */
  void bboxExpand(const glm::vec3 &p, glm::vec3 &bb_min_, glm::vec3 &bb_max);

  /** @brief Worker thread main function */
  static void workerMain(int workerID, Queue<Job>& jobQ, Queue<Result>& resultQ);

  /** @brief Load block from file (for out-of-core rendering) */
  static void loadBlock(const std::filesystem::path& path, const int & count, Result & r);

  /** @brief Load block from file (for in-core rendering) */
  static void loadBlock(const std::filesystem::path& path, const int & count, std::vector<Point>& points);

};
#endif // DATAMANAGER_H
