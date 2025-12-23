#ifndef CACHE_H
#define CACHE_H

#include <filesystem>
#include <list>
#include <fstream>
#include <unordered_map>

struct CachedObj {
    std::ofstream os;
    bool header_written = false;
    std::list<int>::iterator lru_it; // works like a pointer.
};

class Cache{

public:
    Cache();
    bool init(std::filesystem::path dir, size_t cap);
    std::ofstream& get(int id);
    void close_all();

private:
    std::string pathFor(int id) const;
    void touch(int id);
    void evict_one();

    std::filesystem::path outDir;
    size_t capacity;
    std::unordered_map<int, CachedObj> m;
    std::list<int> lru; // front = most recent
};

#endif // CACHE_H
