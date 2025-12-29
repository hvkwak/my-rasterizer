#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"
#include "Point.h"
#include "Plane.h"
#include "Block.h"
#include "DataManager.h"
#include <vector>
#include <filesystem>

class Rasterizer
{
public:
  bool init(const std::filesystem::path& plyPath,
            const std::filesystem::path& outDir,
            const std::filesystem::path& shader_vert,
            const std::filesystem::path& shader_frag,
            bool isTest_,
            bool isOOC_,
            unsigned int window_width_,
            unsigned int window_height_,
            float z_near_,
            float z_far_,
            float rotateAngle_);
  void render();
  Rasterizer();
  ~Rasterizer();

private:

  // setups
  bool setupWindow();
  bool setupShader();
  bool setupRasterizer();
  bool setupCameraPose();
  bool setupCallbacks();
  bool setupDataManager();
  bool setupBufferWrapper();
  bool setupBuffer();
  bool setupBufferPerBlock();
  bool setupCulling();

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

  // updateFPS
  void updateFPS();

  // load, draw, cull Blocks
  void loadBlocks();
  void drawBlocks();
  void cullBlocks();
  int loadedBlocks = 0;

  // Culling
  void aabbIntersectsFrustum(Block & block);
  void buildFrustumPlanes();
  std::array<Plane, 6> planes;

  // initialization parameters
  std::filesystem::path plyPath;
  std::filesystem::path outDir;
  std::filesystem::path shader_vert;
  std::filesystem::path shader_frag;

  // Data Manager
  DataManager dataManager;

  // Bboxes of the loaded scene
  glm::vec3 bb_min;
  glm::vec3 bb_max;
  glm::vec3 center;

  // Shader
  Shader *shader = nullptr;

  // Window
  GLFWwindow *window = nullptr;

  // settings
  unsigned int window_width;
  unsigned int window_height;

  // Camera
  Camera camera;
  glm::vec3 camera_position;
  float diag;
  float lastX /*= WINDOW_WIDTH / 2.0f*/;
  float lastY /*= WINDOW_HEIGHT / 2.0f*/;
  bool firstMouse = true;
  bool hasFocus = true;

  // Transformations
  glm::mat4 model;
  glm::mat4 proj;
  glm::mat4 view;

  // z_near, z_far
  float z_near;
  float z_far;

  // timing to calculate FPS
  float deltaTime = 0.0f; // time between current frame and last frame
  float lastFrame = 0.0f;
  float acc = 0.0f;
  unsigned int frames = 0;
  float angularSpeed;// = glm::radians(10.0f); // 10.0 degrees/sec

  // VBO, VAO for in-core rendering?
  unsigned int VBO = 0, VAO = 0;
  bool glInitialized = false;

  // points for in-core
  std::vector<Point> points;

  // Modes Selection
  bool isTest;
  bool isOOC;

  // blocks for OOC
  std::vector<Block> blocks;
};


#endif // RASTERIZER_H
