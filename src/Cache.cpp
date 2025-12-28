#include "Cache.h"
#include <filesystem>

Cache::Cache(){}

bool Cache::init(std::filesystem::path dir, size_t cap){
  outDir = dir;
  capacity = cap;
  return true;
}

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
  }

  // open it in append mode
  CachedObj co;
  co.os.open(p, std::ios::binary | std::ios::out | std::ios::app);

  lru.push_front(id);
  co.lru_it = lru.begin();
  auto [ins, ok] = m.emplace(id, std::move(co));
  return ins->second.os;
}

void Cache::close_all() {
  for (auto &[id, co] : m)
    co.os.close();
  m.clear();
  lru.clear();
}

void Cache::touch(int id) {
  auto &co = m.at(id);
  lru.erase(co.lru_it);
  lru.push_front(id);
  co.lru_it = lru.begin();
}

void Cache::evict_one() {
  int victim = lru.back();
  lru.pop_back();
  auto it = m.find(victim);
  it->second.os.close();
  m.erase(it);
}
