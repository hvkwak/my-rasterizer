//=============================================================================
//
//   SubslotsCache - LRU cache for out-of-core block management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef SUBSLOTSCACHE_H
#define SUBSLOTSCACHE_H

#include <list>
#include <unordered_map>
#include "Slot.h"

/**
 * @brief LRU cache for out-of-core block management
 *
 * Implements a Least Recently Used cache to store blocks that have been
 * evicted from the primary slots but may be needed again soon.
 */
class SubslotsCache {
public:
    /**
     * @brief Initialize cache with given capacity
     * @param size Maximum number of slots to cache
     * @return true if initialization succeeds
     */
    bool init(int size);

    /**
     * @brief Mark block as recently used (move to MRU position)
     * @param blockID ID of the block to touch
     * @return true if block was found and touched
     */
    bool touch(int blockID);

    /**
     * @brief Insert or update a slot in the cache
     * @param s Slot to insert (moved into cache)
     * @param evicted Optional pointer to receive evicted slot (nullptr if not needed)
     * @return true if a slot was evicted, false otherwise
     */
    bool put(Slot s, Slot* evicted = nullptr);

    /**
     * @brief Evict the least recently used slot
     * @return blockID of the evicted slot
     */
    int evictSubslot();

    /**
     * @brief Extract a slot by blockID, removing it from cache
     * @param blockID ID of the block to extract
     * @param out Output parameter for the extracted slot
     * @return true if block was found and extracted
     */
    bool extract(int blockID, Slot& out);

    /**
     * @brief Clear cache, deleting all VAO/VBO resources
     */
    void clear();

private:
    size_t capacity;
    std::list<Slot> slots;
    std::unordered_map<int, std::list<Slot>::iterator> where;
};

#endif // SUBSLOTSCACHE_H
