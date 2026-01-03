//=============================================================================
//
//   Job - Job and Result structures for multi-threaded data loading
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef JOB_H
#define JOB_H

#include "Point.h"
#include <filesystem>

/**
 * @brief Job structure for worker threads
 */
struct Job {
  int blockID; // block id
  int count;
  int slotIdx;
  bool loadSubSlots;
  std::filesystem::path path;
};

/**
 * @brief Result structure containing loaded block data
 */
struct Result {
  int blockID; // block id
  int slotIdx;
  int count;
  bool loadSubSlots;
  std::vector<Point> points;
};

#endif // JOB_H
