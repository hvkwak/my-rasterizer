#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <string>
#include <vector>
#include <array>
#include <Point.h>
#include <Cache.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int BLOCKS = 1000;
constexpr size_t CACHE_SIZE = 128;
constexpr size_t FLUSH_POINTS = 4096;

class DataHandler {

private:
  struct Block {
    unsigned int id;
    unsigned int vbo = 0, vao = 0;
    unsigned int count = 0;
    unsigned int used = 0;
  };

  void bboxExpand(const glm::vec3 &p, glm::vec3 &bb_min_, glm::vec3 &bb_max);
  void flush(int id, std::array<std::vector<PointOOC>, BLOCKS> &outBuf);
  Cache cache;
  std::vector<Block> blocks;

public:
  DataHandler();

  // sets up DataHandler/Cache
  bool init(const std::string &outDir);

  // Core functionality
  bool createBlocks(const std::string &plyPath, glm::vec3 &bb_min_, glm::vec3 &bb_max_);
  std::vector<Point> readPLY(const std::string &filename, glm::vec3 &bb_min_, glm::vec3 &bb_max_);

  // Reset bounding box to initial state
  void resetBBox();
};

#endif // DATAHANDLER_H
