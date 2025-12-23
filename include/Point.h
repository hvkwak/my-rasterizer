#ifndef POINT_H
#define POINT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Point {
  glm::vec3 pos;
  glm::vec3 color;
};

#pragma pack(push, 1)
struct PointOOC {
  float x, y, z; // 4 bytes each
  uint8_t r, g, b; // 1 byte each
};
#pragma pack(pop)
static_assert(sizeof(PointOOC) == 15);

#pragma pack(push, 1)
struct FilePoint {
  // Points in .ply
  double x, y, z;        // File has 64-bit doubles, 8 bytes each
  unsigned char r, g, b; // File has 8-bit integers
};
#pragma pack(pop)

static_assert(sizeof(FilePoint) == 27);

#endif
