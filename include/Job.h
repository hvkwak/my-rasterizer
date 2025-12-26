#ifndef JOB_H
#define JOB_H

#include "Point.h"

/**
 * @brief Jobs and Results for Worker Threads
 */
struct Result {
  uint32_t id; // block id
  std::vector<Point> points;
};

struct Job {
  uint32_t id; // block id
  uint32_t count;
};


#endif // JOB_H
