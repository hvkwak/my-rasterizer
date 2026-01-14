//=============================================================================
//
//   SubslotsCache - LRU cache for out-of-core block management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "SubslotsCache.h"

/**
 * @brief Initialize cache with given capacity
 */
bool SubslotsCache::init(int size){
  capacity = size;
  return true;
}

/**
 * @brief Mark block as recently used
 */
bool SubslotsCache::touch(int blockID) {
  auto it = where.find(blockID);
  if (it == where.end())
    return false;

  // Move that node to back in O(1)
  slots.splice(slots.end(), slots, it->second);
  return true;
}

/**
 * @brief Insert or update a slot in the cache
 */
void SubslotsCache::put(Slot s) {
  auto it = where.find(s.blockID);

  if (it != where.end()) {
    // Update payload (optional) and move to MRU
    *(it->second) = std::move(s);
    slots.splice(slots.end(), slots, it->second);
    return;
  }

  // New item: add to MRU
  slots.push_back(std::move(s));
  auto nodeIt = std::prev(slots.end());
  where[nodeIt->blockID] = nodeIt;

  // Evict if needed
  if (slots.size() > capacity) {
    int evictID = slots.front().blockID;
    slots.pop_front();
    where.erase(evictID);
  }
}

/**
 * @brief Evict the least recently used slot
 */
int SubslotsCache::evictSubslot() {
  int evictID = slots.front().blockID;
  slots.pop_front();
  where.erase(evictID);
  return evictID;
}

/**
 * @brief Extract a slot by blockID, removing it from cache
 */
bool SubslotsCache::extract(int blockID, Slot& out) {
  auto it = where.find(blockID);
  if (it == where.end())
    return false;

  // Move the slot data out
  out = std::move(*(it->second));

  // Remove from list and map
  slots.erase(it->second);
  where.erase(it);
  return true;
}
