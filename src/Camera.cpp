/* -----------------------------------------------------------------------------
 * Based on code from LearnOpenGL.com | Copyright © Joey de Vries
 * Source: https://github.com/JoeyDeVries/LearnOpenGL/includes/camera.h
 * Licensed under CC BY-NC 4.0: https://creativecommons.org/licenses/by-nc/4.0/
 * ----------------------------------------------------------------------------- */

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"

// constructor with vectors
Camera::Camera(glm::vec3 position /*= glm::vec3(0.0f, 0.0f, 0.0f)*/, UpAxis axis_/*=UpAxis::Z*/, float yaw /*=YAW*/, float pitch /*=PITCH*/)
  : Front(glm::vec3(0.0f, 0.0f, -1.0f)), axis(axis_), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
  Position = position;
  setAxis(axis);
  Yaw = yaw;
  Pitch = pitch;
  updateCameraVectors();
}

void Camera::setAxis(UpAxis axis) {
  WorldUp = (axis == UpAxis::Z) ? glm::vec3(0.0f,0.0f,1.0f) : glm::vec3(0.0f,1.0f,0.0f);
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(Position, Position + Front, Up);
}

// processes input received from any keyboard-like input system. Accepts input
// parameter in the form of camera defined ENUM (to abstract it from windowing
// systems)
void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
      Position += Front * velocity;
    if (direction == BACKWARD)
      Position -= Front * velocity;
    if (direction == LEFT)
      Position -= Right * velocity;
    if (direction == RIGHT)
      Position += Right * velocity;
    if (direction == UP)
      Position += Up * velocity;
    if (direction == DOWN)
      Position -= Up * velocity;
    if (direction == YAW_MINUS){
      Yaw -= velocity*10.0;
      updateCameraVectors();
    }
    if (direction == YAW_PLUS){
      Yaw += velocity*10.0;
      updateCameraVectors();
    }
    if (direction == PITCH_MINUS){
      Pitch -= velocity*10.0;
      updateCameraVectors();
    }
    if (direction == PITCH_PLUS){
      Pitch += velocity*10.0;
      updateCameraVectors();
    }
    // std::cout << "Position: "
    //           << Position.x << ", "
    //           << Position.y << ", "
    //           << Position.z << " |"
    //           << "Yaw: "
    //           << Yaw << " | "
    //           << "Pitch: "
    //           << Pitch
    //           << std::endl;
}

// processes input received from a mouse input system. Expects the offset
// value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::constraintPitch(){
  if (Pitch > 89.0f)
    Pitch = 89.0f;
  if (Pitch < -89.0f)
    Pitch = -89.0f;
}

/**
 * @brief manually changes the camera pose
 *
 * @param position new camera position in world coordinates
 *        target new target that camera position is watching
 */
void Camera::changePose(const glm::vec3 &position, const glm::vec3 &target) {
  Position = position;
  updateYawPitch(target);
  updateCameraVectors();
  // std::cout << "Yaw: " << Yaw << std::endl; // left/right
  // std::cout << "Pitch: " << Pitch << std::endl; // "nodding"
}

/**
 * @brief updates Yaw/Pitch based on the current position when watching the
 * target
 */
void Camera::updateYawPitch(const glm::vec3 &target) {
  glm::vec3 dir = glm::normalize(target - Position);
  if (axis == UpAxis::Z) {
    // safety for asin()
    dir.z = glm::clamp(dir.z, -1.0f, 1.0f);
    Pitch = glm::degrees(asin(dir.z));       // Z is “up”
    Yaw = glm::degrees(atan2(dir.y, dir.x)); // yaw in XY plane
  }else{
    // YUp
    dir.y = glm::clamp(dir.y, -1.0f, 1.0f);
    Pitch = glm::degrees(asin(dir.y));
    Yaw = glm::degrees(atan2(dir.z, dir.x));
  }
}

// processes input received from a mouse scroll-wheel event. Only requires
// input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset) {
  Zoom -= (float)yoffset;
  if (Zoom < 1.0f)
    Zoom = 1.0f;
  if (Zoom > 45.0f)
    Zoom = 45.0f;
}

// calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors(GLboolean constrainPitch/*=true*/) {
  // make sure that when pitch is out of bounds, screen doesn't get flipped
  if (constrainPitch) constraintPitch();
  Front = computeFrontFromYawPitch();

  // shared for both conventions
  Right = glm::normalize(glm::cross(Front, WorldUp));
  Up    = glm::normalize(glm::cross(Right, Front));
}

glm::vec3 Camera::computeFrontFromYawPitch() const {
  float yaw_   = glm::radians(Yaw);
  float pitch_ = glm::radians(Pitch);

  if (axis == UpAxis::Z) {
    // Z-up: yaw in XY plane, pitch toward +Z
    return glm::normalize(glm::vec3(
      cos(yaw_) * cos(pitch_),
      sin(yaw_) * cos(pitch_),
      sin(pitch_)
    ));
  } else {
    // Y-up (LearnOpenGL): yaw around Y, pitch toward +Y
    return glm::normalize(glm::vec3(
      cos(yaw_) * cos(pitch_),
      sin(pitch_),
      sin(yaw_) * cos(pitch_)
    ));
  }
}
