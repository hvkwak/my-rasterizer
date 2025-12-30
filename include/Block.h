#ifndef BLOCK_H
#define BLOCK_H

#include "Point.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int GRID = 10;
constexpr int NUM_BLOCKS = GRID*GRID*GRID;

struct Block {
  unsigned int vbo = 0, vao = 0;
  int count = 0;
  int used = 0;
  bool isVisible = false;
  glm::vec3 bb_min, bb_max;
  std::vector<Point> points;
};


#endif // BLOCK_H
