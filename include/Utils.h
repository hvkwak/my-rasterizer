#ifndef UTILS_H
#define UTILS_H
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

inline int clampi(int v, int lo, int hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

inline void bbox_expand(glm::vec3& mn, glm::vec3& mx, const glm::vec3& p) {
  mn = glm::min(mn, p);
  mx = glm::max(mx, p);
}


// Rx (rotate around X axis by theta radians)
/* glm::mat3 Rx(float theta); */
/* glm::mat3 Ry(float theta); */
/* glm::mat3 YawPitch(float yaw, float pitch); */

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

// example: yaw then pitch (R = Ry * Rx)
inline glm::mat3 RyRx(float theta){
  return Ry(theta) * Rx(theta);
}
#endif // UTILS_H
