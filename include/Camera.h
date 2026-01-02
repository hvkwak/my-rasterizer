//=============================================================================
//
//   Camera - Camera control and view matrix computation
//
//   Based on code from LearnOpenGL.com | Copyright © Joey de Vries
//   Source: https://github.com/JoeyDeVries/LearnOpenGL/includes/camera.h
//   Licensed under CC BY-NC 4.0: https://creativecommons.org/licenses/by-nc/4.0/
//
//   Modified by Hyovin Kwak (2026)
//
//=============================================================================

#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Default camera values
const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{

public:
  // Defines several possible options for camera movement. Used as abstraction
  // to stay away from window-system specific input methods
  enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN, YAW_PLUS, YAW_MINUS, PITCH_PLUS, PITCH_MINUS};
  enum UpAxis{Z, Y};

  // camera Attributes
  glm::vec3 Position;
  glm::vec3 Front;
  glm::vec3 Up;
  glm::vec3 Right;
  glm::vec3 WorldUp;
  UpAxis axis;
  // euler Angles
  float Yaw;
  float Pitch;
  // camera options
  float MovementSpeed;
  float MouseSensitivity;
  float Zoom;


  /** @brief Constructor with initial position and orientation */
  Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), UpAxis axis_ = UpAxis::Z, float yaw = YAW, float pitch = PITCH);

  /** @brief Set the world up axis (Z or Y) */
  void setAxis(UpAxis axis);

  /** @brief Get the view matrix calculated using Euler Angles and LookAt Matrix */
  glm::mat4 GetViewMatrix();

  /** @brief Process keyboard input for camera movement */
  void ProcessKeyboard(Camera_Movement direction, float deltaTime);

  /** @brief Process mouse movement for camera rotation */
  void ProcessMouseMovement(float xoffset, float yoffset);

  /**
   * @brief manually changes the pose
   *
   * @param position new camera position in world coordinates
   *        target new target that camera position is watching
   */
  void changePose(const glm::vec3 &position, const glm::vec3 &target);

  /**
   * @brief updates Yaw/Pitch based on the current position when watching the target
   */
  void updateYawPitch(const glm::vec3 &target);

  /** @brief Constrain pitch angle to prevent camera flipping */
  void constraintPitch();

  /** @brief Process mouse scroll-wheel input for zoom */
  void ProcessMouseScroll(float yoffset);

private:
  /** @brief Update camera vectors from Euler angles */
  void updateCameraVectors(GLboolean constrainPitch=true);

  /** @brief Compute front vector from yaw and pitch */
  glm::vec3 computeFrontFromYawPitch() const;
};
#endif
