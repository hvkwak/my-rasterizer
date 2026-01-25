//=============================================================================
//
//   Slot - Slot structure for out-of-core rendering
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef SLOT_H
#define SLOT_H

typedef enum {
  LOADED, // slot
  LOADING,// slot
  EMPTY
} Status ;

/**
 * @brief Slots for out-of-core rendering
 */
struct Slot {
  int blockID = -1;
  int count = 0;
  Status status = EMPTY;
  unsigned int vbo = 0, vao = 0;
  // std::vector<Point> points;
};
#endif // SLOT_H
