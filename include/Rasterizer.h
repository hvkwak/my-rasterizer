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
#include "Profiler.h"
#include "SubslotsCache.h"
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
  /** @brief Update slot with given block ID */
  bool updateSlotByBlockID(int blockID, int targetIdx);
  /** @brief Check if block is already loaded in a slot */
  bool isBlockInSlot(int blockID);
  float slotFactor; // take slotFactor of total blocks to build slots, e.g. 20%.
  int num_slots = INT_MAX;
  int num_subSlots = INT_MAX;
  int num_points_per_slot = INT_MAX;

  // Block rendering
  /** @brief Clear color and depth buffers */
  void clear();
  /** @brief Set view matrix in shader */
  void setShaderView();
  /** @brief Cull blocks against view frustum */
  void cullBlocks();
  /** @brief sort blocks */
  void sortBlocks();
  /** @brief Draw blocks in in-core mode */
  void drawBlocks();
  /** @brief Load blocks in out-of-core mode */
  void loadBlocksOOC();
  /** @brief Enqueue single block for loading */
  void loadBlock(const int& blockID, const int& slotIdx, const int& count, const bool& loadToSlots);
  /** @brief Draw existing blocks already in slots */
  void drawOldBlocksOOC();
  /** @brief Draw newly loaded blocks from worker threads */
  void drawLoadedBlocksOOC();
  int loadBlockCount = 0;

  // image export
  /** @brief Save framebuffer to PNG file */
  void saveFramePNG(const std::string& path, int w, int h);
  /** @brief Export current frame if export mode enabled */
  void exportFrame();

  // performance check
  /** @brief Update benchmark statistics per frame */
  void updateBenchmarks();
  /** @brief Update window title with FPS info */
  void updateWindowTitle(float fps);
  /** @brief Print benchmark results to stdout */
  void printStats();
  uint64_t vertexCount = 0;
  int visibleCount = 0;
  int limit = 0;
  int cacheMiss = 0;
  int maxCacheMiss = -INT_MAX;
  int minCacheMiss = INT_MAX;
  int maxVisibleCount = -INT_MAX;
  int minVisibleCount = INT_MAX;
  double minFPS = 0.0;
  double maxFPS = 0.0;
  double avgFPS = 0.0;

  glm::vec3 dir;
  /** @brief Set orbital camera pose for test mode */
  void setCameraPose();

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

  // Parameters for benchmarks
  ProfilerCPU profilerCPU;
  const int warmup = 60;
  const int N = 600;
  const float fixedDt = 1.0f / 60.0f;
  float angularSpeed = 0.0f; // glm::radians(10.0f); // this is 10.0 degrees/sec
  float distFactor = 0.0f;

  // initializers
  bool glInitialized = false;
  bool cacheInitialized = false;

  // Modes Selection
  bool isTest;
  bool isOOC;
  bool isCache;
  bool isExport;

  // blocks / slots / cached blocks in subSlot
  std::vector<Block> blocks;
  std::vector<Slot> slots;
  std::vector<unsigned int> vao;
  std::vector<unsigned int> vbo;
  SubslotsCache subSlots;

};


#endif // RASTERIZER_H
