#ifndef BLOCK_H
#define BLOCK_H

constexpr int NUM_BLOCKS = 1000;

struct Block {
  unsigned int vbo = 0, vao = 0;
  int count = 0;
  int used = 0;
  bool isVisible = true;
};


#endif // BLOCK_H
