//=============================================================================
//
//   Profiler - CPU profiling utilities for performance measurement
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef PROFILER_H
#define PROFILER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "Timer.h"

/**
 * @brief CPU profiler for measuring execution time
 */
struct ProfilerCPU {

  /** @brief Statistics for a profiled section */
  struct Stat {
    float total_ms = 0.0f;
    float current_ms = 0.0f;
    int calls = 0;
    float max_ms = 0.0f;
    float min_ms = 0.0f;
  };

  std::unordered_map<std::string, Stat> stats;

  /** @brief Add timing measurement for a named section */
  void add(const std::string& name, float ms) {
    auto& s = stats[name];
    s.total_ms += ms;
    s.current_ms = ms;
    s.calls += 1;
    s.max_ms = std::max(s.max_ms, ms);
    s.min_ms = std::min(s.min_ms, ms);
  }

  /** @brief Print top N profiled sections and reset stats */
  void end_frame_and_print(uint32_t topN = 12) {
    struct Row { std::string name; Stat s; };
    std::vector<Row> rows;
    rows.reserve(stats.size());
    for (auto& kv : stats) rows.push_back({kv.first, kv.second});

    std::sort(rows.begin(), rows.end(), [](auto& a, auto& b){
      return a.s.total_ms > b.s.total_ms;
    });

    std::cout << "---- CPU Profiler ----\n";
    std::cout << std::left << std::setw(14) << "Name"
              << std::right << std::setw(12) << "Total(ms)"
              << std::setw(8) << "Calls"
              << std::setw(12) << "Avg(ms)"
              << std::setw(12) << "Max(ms)"
              << std::setw(12) << "Min(ms)" << "\n";
    std::cout << std::string(70, '-') << "\n";

    for (uint32_t i = 0; i < std::min<uint32_t>(topN, (uint32_t)rows.size()); ++i) {
      const auto& r = rows[i];
      double avg = r.s.total_ms / std::max(1, r.s.calls);
      std::cout << std::left << std::setw(14) << r.name
                << std::right << std::fixed << std::setprecision(3)
                << std::setw(12) << r.s.total_ms
                << std::setw(8) << r.s.calls
                << std::setw(12) << avg
                << std::setw(12) << r.s.max_ms
                << std::setw(12) << r.s.min_ms << "\n";
    }
    stats.clear(); // reset per frame
  }
};

/**
 * @brief RAII wrapper for automatic scope timing
 */
struct ScopedProfile {
  ProfilerCPU& prof;
  TimerCPU t;
  std::string name;
  ScopedProfile(ProfilerCPU& p, std::string n) : prof(p), name(std::move(n)) {}
  ~ScopedProfile() { prof.add(name, t.ms()); }
};


// how to use
/* CpuProfiler gCpuProf; */

/* void RenderFrame() { */
/*   CPU_PROFILE(gCpuProf, "Frame"); */

/*   { CPU_PROFILE(gCpuProf, "Culling");   DoCulling(); } */
/*   { CPU_PROFILE(gCpuProf, "Shadow");    RenderShadow(); } */
/*   { CPU_PROFILE(gCpuProf, "GBuffer");   RenderGBuffer(); } */
/*   { CPU_PROFILE(gCpuProf, "Lighting");  RenderLighting(); } */
/*   { CPU_PROFILE(gCpuProf, "Post");      RenderPost(); } */

/*   gCpuProf.end_frame_and_print(); // 혹은 60프레임마다 출력 */
/* } */

// Macros
#define CONCAT_INNER(a,b) a##b
#define CONCAT(a,b) CONCAT_INNER(a,b)
#define CPU_PROFILE(prof, name) ScopedProfile CONCAT(_ScopedProfile_, __LINE__)(prof, name)

#endif // PROFILER_H
