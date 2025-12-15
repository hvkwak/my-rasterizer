#ifndef DATA_H
#define DATA_H

#include <string>
#include <vector>
#include <Point.h>

std::vector<Point> readPLY(const std::string& filename, glm::vec3 & bb_min, glm::vec3& bb_max);

#endif // DATA_H
