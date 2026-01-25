//=============================================================================
//
//   SubslotsCache - LRU cache for out-of-core block management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "SubslotsCache.h"
#include <glad/glad.h>

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

  // Move that node to the end in O(1)
  slots.splice(slots.end(), slots, it->second);
  return true;
}

/**
 * @brief Insert or update a slot in the cache
 * @param s Slot to insert (moved into cache)
 * @param evicted Optional pointer to receive evicted slot
 * @return true if a slot was evicted, false otherwise
 */
bool SubslotsCache::put(Slot s, Slot* evicted) {
  auto it = where.find(s.blockID);

  if (it != where.end()) {
    // Update existing entry, move to MRU
    *(it->second) = std::move(s);
    slots.splice(slots.end(), slots, it->second);
    return false;
  }

  // Check if we need to evict before adding
  bool willEvict = (slots.size() >= capacity);

  if (willEvict) {
    // Extract LRU slot before evicting
    if (evicted) {
      *evicted = std::move(slots.front());
    }
    where.erase(slots.front().blockID);
    slots.pop_front();
  }

  // Add new item at MRU position
  slots.push_back(std::move(s));
  auto nodeIt = std::prev(slots.end());
  where[nodeIt->blockID] = nodeIt;

  return willEvict;
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

/**
 * @brief Clear cache, deleting all VAO/VBO resources
 */
void SubslotsCache::clear() {
  for (auto& slot : slots) {
    if (slot.vao != 0) glDeleteVertexArrays(1, &slot.vao);
    if (slot.vbo != 0) glDeleteBuffers(1, &slot.vbo);
  }
  slots.clear();
  where.clear();
}
