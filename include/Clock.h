#ifndef CLOCK_H
#define CLOCK_H

#include <chrono>
using steady_clock_t = std::chrono::steady_clock;

static inline double seconds(steady_clock_t::time_point a, steady_clock_t::time_point b) {
  return std::chrono::duration<double>(b - a).count();
}

#endif // CLOCK_H
