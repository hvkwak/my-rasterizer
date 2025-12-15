#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"
#include "Point.h"
#include <vector>


class Rasterizer
{
public:
  bool init(const std::string& read_filename, const std::string& read_shader_vert, const std::string& read_shader_frag);
  void render();
  Rasterizer();
  ~Rasterizer();

private:

  // setups
  bool setupWindow();
  bool setupShader(const std::string& read_shader_vert, const std::string& read_shader_frag);
  bool setupRasterizer();
  bool setupCameraPose();
  bool setupInput();
  bool setupData(const std::string& read_filename);
  bool setupBuffer();

  // Shader
  Shader *shader = nullptr;

  // Window
  GLFWwindow *window = nullptr;

  // key pressings
  void processInput();

  // input handlers
  void handleMouseMove(GLFWwindow* window, double xposIn, double yposIn);
  void handleWindowFocus(GLFWwindow* window, int focused);

  // callbacks
  static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
  static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
  static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
  static void window_focus_callback(GLFWwindow* window, int focused);

  // settings
  const unsigned int WINDOW_WIDTH = 800;
  const unsigned int WINDOW_HEIGHT = 600;

  // Camera
  Camera camera;
  glm::vec3 camera_position;
  float lastX = WINDOW_WIDTH / 2.0f;
  float lastY = WINDOW_HEIGHT / 2.0f;
  bool firstMouse = true;
  bool hasFocus = true;

  // z_near, z_far
  const float z_near = 1.0;
  const float z_far = 100.0;

  // timing
  float deltaTime = 0.0f; // time between current frame and last frame
  float lastFrame = 0.0f;

  // VBO, VAO
  unsigned int VBO = 0, VAO = 0;
  bool glInitialized = false;

  // points
  std::vector<Point> points;

  // bb_min, bb_max of the scene
  glm::vec3 bb_min;
  glm::vec3 bb_max;

};


#endif // RASTERIZER_H
