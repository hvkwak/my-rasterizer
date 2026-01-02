//=============================================================================
//
//   Plane - Plane representation for frustum culling
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef PLANE_H
#define PLANE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Plane {
  glm::vec3 n; // normal
  float d;     // plane eq: dot(n, x) + d >= 0 is inside
};

/** @brief Normalize plane equation from vec4 representation */
Plane normalizePlane(const glm::vec4& p);

#endif // PLANE_H
