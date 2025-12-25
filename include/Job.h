#ifndef JOB_H
#define JOB_H

/**
 * @brief Jobs and Results for Worker Threads
 */
struct Job {
  int id; // block id for load
};

struct Result {
  int id; // block id for visualization
  // std::vector<Point>
};

#endif // JOB_H
