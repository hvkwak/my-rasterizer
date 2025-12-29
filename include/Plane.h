#ifndef PLANE_H
#define PLANE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Plane {
  glm::vec3 n; // normal
  float d;     // plane eq: dot(n, x) + d >= 0 is inside
};

Plane normalizePlane(const glm::vec4& p);

#endif // PLANE_H
