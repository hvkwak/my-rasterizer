#ifndef DATAHANDLER_H
#define DATAHANDLER_H

#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <Point.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr int BLOCKS = 1000;
constexpr size_t FLUSH_POINTS = 4096;

class DataHandler {
private:

    // Private helper methods
    void bboxExpand(const glm::vec3& p, glm::vec3& bb_min_, glm::vec3& bb_max);
    void flush(int id, std::vector<std::ofstream>& outs, std::array<std::vector<PointDisk>, BLOCKS>& outBuf);

public:
    // Constructor
    DataHandler();

    // Getters for bounding box
    // const glm::vec3& getBBoxMin() const;
    // const glm::vec3& getBBoxMax() const;

    // Core functionality
    bool createBlocks(const std::string& plyPath, const std::string& outDir, glm::vec3& bb_min_, glm::vec3& bb_max_);
    std::vector<Point> readPLY(const std::string& filename, glm::vec3& bb_min_, glm::vec3& bb_max_);

    // Reset bounding box to initial state
    void resetBBox();
};

#endif // DATAHANDLER_H
