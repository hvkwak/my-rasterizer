#include "Plane.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Plane normalizePlane(const glm::vec4& p) {
  Plane out;
  glm::vec3 n(p.x, p.y, p.z);
  float len = glm::length(n);
  out.n = n / len;
  out.d = p.w / len;
  return out;
}
