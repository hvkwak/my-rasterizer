//=============================================================================
//
//   FileStreamCache - LRU cache for file stream management
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef FILE_STREAM_CACHE_H
#define FILE_STREAM_CACHE_H

#include <filesystem>
#include <list>
#include <fstream>
#include <unordered_map>

struct CachedObj {
    std::ofstream os;
    bool header_written = false;
    std::list<int>::iterator lru_it;
};

class FileStreamCache{

public:
    FileStreamCache();

    /** @brief Initialize cache with given capacity */
    bool init(size_t cap);

    /** @brief Get file stream for given ID, opening if necessary */
    std::ofstream& get(int id, const std::filesystem::path& p);

    /** @brief Close all open file streams */
    void close_all();

private:
    /** @brief Mark ID as most recently used */
    void touch(int id);

    /** @brief Evict least recently used entry */
    void evict_one();

    std::filesystem::path outDir;
    size_t capacity;
    std::unordered_map<int, CachedObj> m;
    std::list<int> lru; // front = most recent
};

#endif // FILE_STREAM_CACHE_H
