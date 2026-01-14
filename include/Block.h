//=============================================================================
//
//   Block - Spatial block structure for point cloud partitioning
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef BLOCK_H
#define BLOCK_H

#include "Point.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int GRID = 10;
constexpr int NUM_BLOCKS = GRID*GRID*GRID;

/**
 * @brief Spatial block for point cloud partitioning
 */
struct Block {
  int blockID = -1;
  int count = 0;
  bool isVisible = false;
  glm::vec3 bb_min, bb_max;
  float distanceToPlaneMin = 0.0f;
  float distanceToCameraCenter = 0.0f;
  float distanceToFrustumCenter = 0.0f;

  // used only in in-core mode
  unsigned int vbo = 0, vao = 0;
  std::vector<Point> points;
};


#endif // BLOCK_H
