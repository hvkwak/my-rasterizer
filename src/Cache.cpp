//=============================================================================
//
//   Cache - LRU cache for file stream management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "Cache.h"
#include <filesystem>
#include <iostream>

Cache::Cache(){}

/**
 * @brief Initialize cache with given capacity
 */
bool Cache::init(size_t cap){
  capacity = cap;
  return true;
}

/**
 * @brief Get file stream for given ID, opening if necessary
 */
std::ofstream& Cache::get(int id, const std::filesystem::path& p) {
  auto it = m.find(id); // returns an iterator pointing to the soughth after element.
  if (it != m.end()) {
    touch(id);
    return it->second.os; // second == CachedObj
  }

  if (m.size() >= capacity){
    evict_one();
  }

  // create a new file if it not exists
  if (!std::filesystem::exists(p)) {
    std::ofstream tmp(p, std::ios::binary | std::ios::out);

    /*ADDED ERROR HANDLING*/
    if (!tmp.is_open() || !tmp.good()) {
      std::cerr << "Error: Failed to create block file: " << p << std::endl;
    }
  }

  // open it in append mode
  CachedObj co;
  co.os.open(p, std::ios::binary | std::ios::out | std::ios::app);

  /*ADDED ERROR HANDLING*/
  if (!co.os.is_open()) {
    std::cerr << "Error: Failed to open block file for writing: " << p << std::endl;
  }

  lru.push_front(id);
  co.lru_it = lru.begin();
  auto [ins, ok] = m.emplace(id, std::move(co));
  return ins->second.os;
}

/**
 * @brief Close all open file streams
 */
void Cache::close_all() {
  for (auto &[id, co] : m) {
    co.os.close();

    /*ADDED ERROR HANDLING*/
    if (co.os.fail()) {
      std::cerr << "Warning: Error closing cache entry (block " << id << ")" << std::endl;
    }
  }
  m.clear();
  lru.clear();
}

/**
 * @brief Mark ID as most recently used
 */
void Cache::touch(int id) {
  auto &co = m.at(id);
  lru.erase(co.lru_it);
  lru.push_front(id);
  co.lru_it = lru.begin();
}

/**
 * @brief Evict least recently used entry
 */
void Cache::evict_one() {
  int victim = lru.back();
  lru.pop_back();
  auto it = m.find(victim);
  it->second.os.close();

  /*ADDED ERROR HANDLING*/
  if (it->second.os.fail()) {
    std::cerr << "Warning: Error closing evicted cache entry (block " << victim << ")" << std::endl;
  }

  m.erase(it);
}
