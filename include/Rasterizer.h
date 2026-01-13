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
            bool isCache_,
            bool isExport_,
            unsigned int window_width_,
            unsigned int window_height_,
            float z_near_,
            float z_far_,
            float rotateAngle_,
            float distanceFactor_,
            float slotFactor_);

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

  // Slot management
  /** @brief Find slot index containing given block ID */
  bool updateSlotByBlockID(std::vector<Slot>& slotsSource, std::vector<Slot>& slotsTarget, int blockID, int slotIdx);
  bool isBlockInSlot(std::vector<Slot>& slotsA, int blockID);
  float slotFactor; // take slotFactor of total blocks to build slots, e.g. 20%.
  int num_slots;
  int num_subSlots;
  int num_points_per_slot;

  // Block rendering
  /** @brief Cull blocks against view frustum */
  void cullBlocks();
  /** @brief Draw blocks in in-core mode */
  void drawBlocks();
  /** @brief Load blocks in out-of-core mode */
  void loadBlocksOOC();
  void loadBlock(const int& blockID, const int& slotIdx, const int& count, const bool& loadSubSlots);
  /** @brief Draw blocks in out-of-core mode */
  void drawBlocksOOC();
  int loadBlockCount = 0;
  int cacheHitsA = 0;
  int cacheHitsB = 0;

  // image export
  void saveFramePNG(const std::string& path, int w, int h);

  // performance check variables
  uint64_t vertexCount = 0;
  int visibleCount = 0;
  int cacheMiss = 0;
  int maxCacheMiss = -INT_MAX;
  int minCacheMiss = INT_MAX;
  int maxVisibleCount = -INT_MAX;
  int minVisibleCount = INT_MAX;

  /** @brief Set orbital camera pose for test mode */
  glm::vec3 dir;
  void setOrbitCamera();

  // Frustum culling
  /** @brief Test if AABB intersects view frustum */
  void aabbIntersectsFrustum(Block & block);
  /** @brief Build frustum planes from projection matrix */
  void buildFrustumPlanes();
  std::array<Plane, 6> planes;
  float margin = 30.0; // culling margin

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

  // Parameters for benchmarks
  const int warmup = 60;
  const int N = 600;
  const float fixedDt = 1.0f / 60.0f;
  double minFPS_N = std::numeric_limits<double>::infinity();
  double maxFPS_N = 0.0;
  double totalSeconds = 0.0;
  int frameIdx = 0;
  float angularSpeed = 0.0f;// = glm::radians(10.0f); // 10.0 degrees/sec
  float distFactor = 0.0f;

  // initializers
  bool glInitialized = false;
  bool cacheInitialized = false;

  // Modes Selection
  bool isTest;
  bool isOOC;
  bool isCache;
  bool isExport;

  // blocks / slots / cached blocks in slot
  std::vector<Block> blocks;
  std::vector<Slot> slots;
  std::vector<unsigned int> vao;
  std::vector<unsigned int> vbo;
  std::vector<Slot> subSlots;
};


#endif // RASTERIZER_H
