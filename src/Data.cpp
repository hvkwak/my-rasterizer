#include "Data.h"
#include "Point.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

/**
 * @brief reads .ply binary point cloud data
 *
 * @param filename filename of .ply file
 * @return vector of 3D points
 */
std::vector<Point> readPLY(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<Point> finalPoints;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return finalPoints;
    }

    // --- Phase 1: Parse Header ---
    std::string line;
    long vertexCount = 0;
    bool headerEnded = false;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        if (word == "element") {
            ss >> word;
            if (word == "vertex") ss >> vertexCount;
        }
        else if (word == "end_header") {
            headerEnded = true;
            break;
        }
    }

    if (!headerEnded || vertexCount == 0) return finalPoints;

    // --- Phase 2: Read Binary Data ---

    // 1. Create a buffer to hold the raw file data
    std::vector<FilePoint> rawData(vertexCount);

    // 2. Read the file into the buffer in one go (very fast)
    file.read(reinterpret_cast<char*>(rawData.data()), vertexCount * sizeof(FilePoint));

    // --- Phase 3: Convert to your GLM format ---

    // Reserve memory for your actual points
    finalPoints.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {
        // Convert Double (File) -> Float (GLM)
        finalPoints[i].pos = glm::vec3(
            static_cast<float>(rawData[i].x),
            static_cast<float>(rawData[i].y),
            static_cast<float>(rawData[i].z)
        );

        // Convert Uchar [0-255] -> Float [0.0-1.0] (GLM Standard)
        finalPoints[i].color = glm::vec3(
            static_cast<float>(rawData[i].r) / 255.0f,
            static_cast<float>(rawData[i].g) / 255.0f,
            static_cast<float>(rawData[i].b) / 255.0f
        );
    }

    std::cout << "Successfully converted " << finalPoints.size() << " points." << std::endl;
    return finalPoints;
}
