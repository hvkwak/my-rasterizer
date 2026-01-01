#ifndef JOB_H
#define JOB_H

#include "Point.h"
#include <filesystem>

/**
 * @brief Jobs and Results for Worker Threads
 */

struct Job {
  int blockID; // block id
  int count;
  int slotIdx;
  std::filesystem::path path;
};

struct Result {
  int blockID; // block id
  int slotIdx;
  int count;
  std::vector<Point> points;
};

#endif // JOB_H
