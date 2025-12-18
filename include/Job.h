#ifndef JOB_H
#define JOB_H

#include <string>


/**
 * @brief Jobs for Worker Threads
 */
struct Job {
  int id;
  std::string payload;
};

struct Result {
  int job_id;
  std::string message;
};

#endif // JOB_H
