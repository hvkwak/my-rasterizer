//=============================================================================
//
//   Utils - Utility functions for math and transformations
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef UTILS_H
#define UTILS_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/** @brief Clamp integer value to range [lo, hi] */
inline int clampi(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

/** @brief Expand bounding box to include point */
inline void bbox_expand(glm::vec3& mn, glm::vec3& mx, const glm::vec3& p) {
  mn = glm::min(mn, p);
  mx = glm::max(mx, p);
}

/** @brief Rotation matrix around Z axis */
inline glm::mat3 Rz(float theta)
{
  float c = std::cos(theta);
  float s = std::sin(theta);
  // GLM is column-major: mat3(col0, col1, col2)
  return glm::mat3(
    glm::vec3( c,  s, 0),
    glm::vec3(-s,  c, 0),
    glm::vec3( 0,  0, 1)
  );
}

/** @brief Rotation matrix around X axis */
inline glm::mat3 Rx(float theta)
{
  float c = std::cos(theta);
  float s = std::sin(theta);
  // GLM is column-major: mat3(col0, col1, col2)
  return glm::mat3(
    glm::vec3(1, 0, 0),
    glm::vec3(0, c, s),
    glm::vec3(0,-s, c)
  );
}

/** @brief Rotation matrix around Y axis */
inline glm::mat3 Ry(float phi)
{
  float c = std::cos(phi);
  float s = std::sin(phi);
  return glm::mat3(
    glm::vec3( c, 0,-s),
    glm::vec3( 0, 1, 0),
    glm::vec3( s, 0, c)
  );
}

/** @brief Combined rotation: yaw then pitch (R = Ry * Rx) */
inline glm::mat3 RyRx(float theta){
  return Ry(theta) * Rx(theta);
}
#endif // UTILS_H
