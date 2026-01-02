//=============================================================================
//
//   Rasterizer - Main rendering engine for point cloud visualization
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Camera.h"
#include "Plane.h"
#include "Block.h"
#include "Slot.h"
#include "DataManager.h"
#include <vector>
#include <filesystem>

class Rasterizer
{
public:
  /**
   * @brief Initialize the rasterizer with rendering parameters
   * @return true if initialization succeeds, false otherwise
   */
  bool init(const std::filesystem::path& plyPath,
            const std::filesystem::path& outDir,
            const std::filesystem::path& shader_vert,
            const std::filesystem::path& shader_frag,
            bool isTest_,
            bool isOOC_,
            bool isSlot_,
            bool isCache_,
            unsigned int window_width_,
            unsigned int window_height_,
            float z_near_,
            float z_far_,
            float rotateAngle_,
            float distanceFactor_);

  /**
   * @brief Main render loop
   */
  void render();

  Rasterizer();
  ~Rasterizer();

private:

  // Setup functions
  /** @brief Setup GLFW window */
  bool setupWindow();
  /** @brief Setup shader program */
  bool setupShader();
  /** @brief Initialize OpenGL settings */
  bool setupRasterizer();
  /** @brief Initialize camera position based on scene bounds */
  bool setupCameraPose();
  /** @brief Setup input callbacks */
  bool setupCallbacks();
  /** @brief Initialize data manager */
  bool setupDataManager();
  /** @brief Filter out empty blocks */
  bool filterBlocks();
  /** @brief Setup buffer wrapper (in-core or out-of-core) */
  bool setupBufferWrapper();
  /** @brief Setup OpenGL buffers per block for in-core rendering */
  bool setupBufferPerBlock();
  /** @brief Setup frustum culling */
  bool setupCulling();
  /** @brief Setup slots for out-of-core rendering */
  bool setupSlots();

  // Input handling
  /** @brief Process keyboard input */
  void processInput();
  /** @brief Handle mouse movement */
  void handleMouseMove(GLFWwindow* window, double xposIn, double yposIn);
  /** @brief Handle window focus changes */
  void handleWindowFocus(GLFWwindow* window, int focused);

  // GLFW callbacks
  static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
  static void framebuffer_size_callback(GLFWwindow *window, int width, int height);
  static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
  static void window_focus_callback(GLFWwindow* window, int focused);

  /** @brief Update FPS and frame statistics */
  void updateInfo();

  // Slot management
  /** @brief Initialize rendering slots */
  void initSlots(std::array<Slot, NUM_SLOTS>& slots);
  /** @brief Find slot index containing given block ID */
  int findSlot(int blockID);

  // Block rendering
  /** @brief Cull blocks against view frustum */
  void cullBlocks();
  /** @brief Draw blocks in in-core mode */
  void drawBlocks();
  /** @brief Load blocks in out-of-core mode */
  void loadBlocksOOC();
  /** @brief Draw blocks in out-of-core mode */
  void drawBlocksOOC();
  int loadedBlocks = 0;
  int loadingBlocks = 0;
  int cachedBlocks = 0;
  int visibleBlocks = 0;
  int maxVisibleBlocks = -INT_MAX;
  int minVisibleBlocks = INT_MAX;

  /** @brief Set orbital camera pose for test mode */
  void setOrbitCamera();

  // Frustum culling
  /** @brief Test if AABB intersects view frustum */
  void aabbIntersectsFrustum(Block & block);
  /** @brief Build frustum planes from projection matrix */
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
  bool isFPSInitialized = false;
  float maxFPS = -FLT_MAX;
  float minFPS = FLT_MAX;
  float acc = 0.0f;
  unsigned int frames = 0;
  float angularSpeed = 0.0f;// = glm::radians(10.0f); // 10.0 degrees/sec
  float distFactor = 0.0f;

  bool glInitialized = false;

  // Modes Selection
  bool isTest;
  bool isOOC;

  // blocks / slots / cached blocks in slot
  std::vector<Block> blocks;
  std::vector<Slot> slots;
  std::vector<Slot> subSlots;
};


#endif // RASTERIZER_H
