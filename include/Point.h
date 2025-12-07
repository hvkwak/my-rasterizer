#ifndef POINT_H
#define POINT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Point {
    glm::vec3 pos;
    glm::vec3 color;
};

#pragma pack(push, 1)
struct FilePoint {
    double x, y, z;        // File has 64-bit doubles
    unsigned char r, g, b; // File has 8-bit integers
};
#pragma pack(pop)

#endif
