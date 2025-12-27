#ifndef JOB_H
#define JOB_H

#include "Point.h"

/**
 * @brief Jobs and Results for Worker Threads
 */

struct Job {
  uint32_t blockID; // block id
  uint32_t count;
};

struct Result {
  uint32_t blockID; // block id
  std::vector<Point> points;
};

#endif // JOB_H
