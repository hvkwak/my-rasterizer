//=============================================================================
//
//   Plane - Plane representation for frustum culling
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "Plane.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/**
 * @brief Normalize plane equation from vec4 representation
 * @param p Plane equation as vec4 (n.x, n.y, n.z, d)
 * @return Normalized plane with unit normal vector
 */
Plane normalizePlane(const glm::vec4& p) {
  Plane out;
  glm::vec3 n(p.x, p.y, p.z);
  float len = glm::length(n);
  out.n = n / len;
  out.d = p.w / len;
  return out;
}
