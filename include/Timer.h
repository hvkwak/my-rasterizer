//=============================================================================
//
//   Timer - CPU timer
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

/**
 * @brief High resolution CPU timer using steady_clock
 */
struct TimerCPU {
  using clock = std::chrono::steady_clock;
  clock::time_point t0 = clock::now();

  /** @brief Reset timer to current time */
  void reset() { t0 = clock::now(); }

  /** @brief Get elapsed time in milliseconds */
  float ms() const {
    return std::chrono::duration<float, std::milli>(clock::now() - t0).count();
  }
};

#endif // TIMER_H
