//=============================================================================
//
//   Point - Point data structures for rendering and file I/O
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef POINT_H
#define POINT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Point structure for rendering
 */
struct Point {
  glm::vec3 pos;
  glm::vec3 color;
};
static_assert(alignof(glm::vec3) == alignof(float));
static_assert(alignof(Point) == alignof(float));
static_assert(sizeof(Point) == 24);

/**
 * @brief Point structure as stored in PLY file
 */
#pragma pack(push, 1)
struct FilePoint {
  double x, y, z;        // File has 64-bit doubles, 8 bytes each
  unsigned char r, g, b; // File has 8-bit integers
};
#pragma pack(pop)
static_assert(sizeof(FilePoint) == 27);



/* #pragma pack(push, 1) */
/* struct PointOOC { */
/*   float x, y, z; // 4 bytes each */
/*   uint8_t r, g, b; // 1 byte each */
/* }; */
/* #pragma pack(pop) */
/* static_assert(sizeof(PointOOC) == 15); */

#endif
