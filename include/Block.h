#ifndef BLOCK_H
#define BLOCK_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int NUM_BLOCKS = 1000;
constexpr int GRID = 10;

struct Block {
  unsigned int vbo = 0, vao = 0;
  int count = 0;
  int used = 0;
  bool isVisible = false;
  glm::vec3 bb_min, bb_max;

};


#endif // BLOCK_H
