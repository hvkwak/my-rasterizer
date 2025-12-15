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
std::vector<Point> readPLY(const std::string& filename, glm::vec3 & bb_min, glm::vec3& bb_max) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<Point> finalPoints;

    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return finalPoints;
    }

    // parse header
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

    // read binary data
    // 1. Create a buffer to hold the raw file data
    std::vector<FilePoint> rawData(vertexCount);

    // 2. Read the file into the buffer in one go (very fast)
    file.read(reinterpret_cast<char*>(rawData.data()), vertexCount * sizeof(FilePoint));

    if (file.gcount() != vertexCount * sizeof(FilePoint)) {
      std::cerr << "Error: Corrupted file or unexpected end of file." << std::endl;
      finalPoints.clear();
      return finalPoints;
    }

    // convert to GLM format
    // Reserve memory for your actual points
    finalPoints.resize(vertexCount);

    for (size_t i = 0; i < vertexCount; ++i) {

        // Convert Double (File) -> Float (GLM)
        FilePoint point = rawData[i];
        float x = static_cast<float>(point.x);
        float y = static_cast<float>(point.y);
        float z = static_cast<float>(point.z);
        finalPoints[i].pos = glm::vec3(x, y, z);

        // update bb_min, bb_max
        if (x < bb_min.x){bb_min.x = x;}
        if (y < bb_min.y){bb_min.y = y;}
        if (z < bb_min.z){bb_min.z = z;}
        if (x > bb_max.x){bb_max.x = x;}
        if (y > bb_max.y){bb_max.y = y;}
        if (z > bb_max.z){bb_max.z = z;}

        // Convert Uchar [0-255] -> Float [0.0-1.0] (GLM Standard)
        finalPoints[i].color = glm::vec3(
            static_cast<float>(point.r) / 255.0f,
            static_cast<float>(point.g) / 255.0f,
            static_cast<float>(point.b) / 255.0f
        );
    }

    std::cout << "Successfully converted " << finalPoints.size() << " points." << std::endl;
    return finalPoints;
}
