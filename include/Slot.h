#ifndef SLOT_H
#define SLOT_H

#include "Point.h"

constexpr int NUM_SLOTS = 200;
constexpr int NUM_SUB_SLOTS = 16;
constexpr int NUM_POINTS_PER_SLOT = 200000; // capacity

typedef enum {
  LOADED,
  LOADING,
  CACHED,
  EMPTY
} Status ;

struct Slot {
  int blockID = -1;
  int count = 0;
  Status status = EMPTY;

  // used in out-of-core mode
  unsigned int vbo = 0, vao = 0;
  std::vector<Point> points;
};
#endif // SLOT_H
