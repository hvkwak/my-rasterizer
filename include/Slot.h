#ifndef SLOT_H
#define SLOT_H

constexpr int NUM_SLOTS = 64;
constexpr int NUM_SUB_SLOTS = 16;
constexpr int NUM_POINTS_PER_SLOT = 2000; // capacity



struct Slot {
  unsigned int vao = 0;
  unsigned int vbo = 0;

  int blockID = -1;
  int count = 0;
  int lastUsedFrame = 0;
};
#endif // SLOT_H
