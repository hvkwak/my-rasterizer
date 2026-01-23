//=============================================================================
//
//   Rasterizer - Main rendering engine for point cloud visualization
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "Rasterizer.h"
#include <stb_image_write.h>
#include <stb_image.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "DataManager.h"
#include "Shader.h"
#include "Utils.h"
#include <iostream>
#include <cstdio>
#include <cfloat>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Rasterizer::Rasterizer(){}

/**
 * @brief Destructor - cleanup OpenGL resources and GLFW
 */
Rasterizer::~Rasterizer(){
  if (shader != nullptr){
    if (glInitialized && shader->ID != 0) glDeleteProgram(shader->ID);
    delete shader;
  }

  if (window != nullptr) {
    glfwDestroyWindow(window);
    window = nullptr;
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
}

/**
 * @brief Initialize the rasterizer with rendering parameters
 * @return true if initialization succeeds, false otherwise
 */
bool Rasterizer::init(const std::filesystem::path& plyPath_,
                      const std::filesystem::path& outDir_,
                      const std::filesystem::path& shader_vert_,
                      const std::filesystem::path& shader_frag_,
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
                      float slotFactor_){
  // Set member variables
  plyPath = plyPath_;
  outDir = outDir_;
  shader_vert = shader_vert_;
  shader_frag = shader_frag_;
  isTest = isTest_;
  isOOC = isOOC_;
  isCache = isCache_;
  isExport = isExport_;
  window_width = window_width_;
  window_height = window_height_;
  z_near = z_near_;
  z_far = z_far_;
  angularSpeed = glm::radians(rotateAngle_);
  distFactor = distanceFactor_; // for orbital camera pose in test mode
  slotFactor = slotFactor_;
  blocks.resize(NUM_BLOCKS);

  // setups
  if (!setupWindow()) return false;
  if (!setupDataManager()) return false;
  if (!filterBlocks()) return false;
  if (!setupRasterizer()) return false;
  if (!setupCameraPose()) return false;
  if (!setupCallbacks()) return false;
  if (!setupShader()) return false; // before setupCulling()
  if (!setupCulling()) return false;
  if (!setupBufferWrapper()) return false;
  return true;
}


/**
 * @brief loads glad, enable depth test, and set point size
 */
bool Rasterizer::setupRasterizer(){
  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return false;
  }
  glInitialized = true;
  glViewport(0, 0, window_width, window_height);
  glEnable(GL_DEPTH_TEST);
  glPointSize(1.0f);
  return true;
}

/**
 * @brief Initialize camera position based on scene bounds
 */
bool Rasterizer::setupCameraPose(){

  center = 0.5f * (bb_max + bb_min);
  diag = glm::length((bb_max - bb_min));

  // Put camera: above (+Y) and in front (+Z or -Z depending on your world)
  // Take 5% of diag
  camera_position = center + glm::vec3(0.05f * diag, 0.05f * diag, 0.05f * diag);
  camera.changePose(camera_position, center);
  return true;
}

/**
 * @brief Build and compile shader program
 * @return true if shader setup succeeds, false otherwise
 */
bool Rasterizer::setupShader(){
  // build and compile our shader program
  try {
    shader = new Shader(shader_vert.c_str(), shader_frag.c_str()); // keep it like this :(
  } catch (const std::exception& e) {
    std::cerr << "Failed to create Shader: " << e.what() << std::endl;
    shader = nullptr;
    return false;
  }

  GLint linked = 0;
  glGetProgramiv(shader->ID, GL_LINK_STATUS, &linked);
  if (!linked) {
    std::cerr << "Failed to link shader program." << std::endl;
    return false;
  }

  shader->use();
  model = glm::mat4(1.0f); // identity for now
  proj = glm::perspective(glm::radians(45.0f), (float)window_width / (float)window_height, z_near, z_far);
  shader->setMat4("Model", model);
  shader->setMat4("Proj", proj);
  return true;
}

/**
 * @brief Initialize frustum culling
 * @return true if culling setup succeeds
 */
bool Rasterizer::setupCulling(){
  buildFrustumPlanes();
  return true;
}

/**
 * @brief Initialize GLFW window
 * @return true if window creation succeeds, false otherwise
 */
bool Rasterizer::setupWindow(){
  // glfw: initialize and configure
  if (glfwInit() == GLFW_FALSE) {
    std::cerr << "Failed to initialize GLFW" << std::endl;
    return false;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // glfw window creation
  window = glfwCreateWindow(window_width, window_height, "MyRasterizer", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    window = nullptr;
    return false;
  }
  //
  glfwMakeContextCurrent(window);
  glfwSetWindowUserPointer(window, this);
  glfwSwapInterval(0);
  return true;
}

/**
 * @brief Setup input callbacks
 */
bool Rasterizer::setupCallbacks(){
  if (window == nullptr){
    return false;
  }
  glfwSetFramebufferSizeCallback(window, Rasterizer::framebuffer_size_callback);
  glfwSetCursorPosCallback(window, Rasterizer::mouse_callback);
  glfwSetWindowFocusCallback(window, Rasterizer::window_focus_callback);
  glfwSetScrollCallback(window, Rasterizer::scroll_callback);
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(window, &xpos, &ypos);
  lastX = static_cast<float>(xpos);
  lastY = static_cast<float>(ypos);
  firstMouse = true;
  return true;
}

/**
 * @brief Initialize data manager and load point cloud data
 * @return true if data manager initialization succeeds, false otherwise
 */
bool Rasterizer::setupDataManager(){

  // Reset bounding box to initial state
  bb_min = glm::vec3(std::numeric_limits<float>::max());
  bb_max = glm::vec3(std::numeric_limits<float>::lowest());

  // initialize Data Manager
  if(!dataManager.init(plyPath, outDir, isOOC, bb_min, bb_max, blocks, vertexCount)){
    std::cerr << "Error: DataManager.init(). Exiting." << std::endl;
    return false;
  }

  std::cout << "bb_min: "
            << bb_min.x << ", "
            << bb_min.y << ", "
            << bb_min.z << ", "
            << "\n";
  std::cout << "bb_max: "
            << bb_max.x << ", "
            << bb_max.y << ", "
            << bb_max.z << ", "
            << std::endl;
  return true;
}

/**
 * @brief Filter out empty blocks
 */
bool Rasterizer::filterBlocks(){
  size_t k = 0;
  while (k < blocks.size()) {
    if (blocks[k].count == 0) {
      blocks.erase(blocks.begin() + k); // no k++
    } else {
      k++;
    }
  }
  std::cout << "Filtered empty blocks from " << NUM_BLOCKS << " to " << blocks.size() << " blocks." << std::endl;
  return true;
}

/**
 * @brief Setup buffer wrapper for in-core or out-of-core rendering
 * @return true if buffer setup succeeds
 */
bool Rasterizer::setupBufferWrapper(){
  return isOOC ? setupSlots() : setupBufferPerBlock();
}

/**
 * @brief Setup OpenGL buffers per block for in-core rendering
 * @return true if setup is successful
 */
bool Rasterizer::setupBufferPerBlock(){

  for (int i = 0; i < blocks.size(); i++) {
    glGenVertexArrays(1, &blocks[i].vao);
    glGenBuffers(1, &blocks[i].vbo);

    glBindVertexArray(blocks[i].vao);
    glBindBuffer(GL_ARRAY_BUFFER, blocks[i].vbo);

    // set buffer data
    glBufferData(GL_ARRAY_BUFFER, blocks[i].count * sizeof(Point),
                 blocks[i].points.data(), GL_STATIC_DRAW);

    // attrib setup once per VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point),
                          (void *)(sizeof(glm::vec3))); // adjust field name
    glEnableVertexAttribArray(1);
  }
  return true;
}

/**
 * @brief Setup slots for out-of-core rendering
 * @return true if slot setup succeeds
 */
bool Rasterizer::setupSlots() {

  // init slots
  num_slots = (int)(slotFactor * blocks.size());
  num_subSlots = (int)(0.5f*slotFactor * blocks.size());
  num_points_per_slot = (int)(vertexCount/blocks.size());
  std::cout << "num_slots: " << num_slots << "\n";
  std::cout << "num_subSlots: " << num_subSlots << "\n";
  std::cout << "num_points_per_slot: " << num_points_per_slot << "\n";
  slots.resize(num_slots);
  vao.resize(num_slots);
  vbo.resize(num_slots);
  // block_to_slot.clear();
  // block_to_slot.reserve(num_slots);

  if (isCache){
    subSlots.init(num_subSlots);
  }

  for (int i = 0; i < num_slots; ++i) {
    glGenVertexArrays(1, &vao[i]);
    glGenBuffers(1, &vbo[i]);

    glBindVertexArray(vao[i]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);

    // set the capacity per slot, we use glBufferSubData
    glBufferData(GL_ARRAY_BUFFER,
                 num_points_per_slot * sizeof(Point),
                 nullptr,
                 GL_STREAM_DRAW);

    // attrib 0: position (vec3)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);

    // attrib 1: color (vec3)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)(sizeof(glm::vec3)));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }
  return true;
}



/**
 * @brief Extract frustum planes from projection matrix
 */
void Rasterizer::buildFrustumPlanes() {

  // GLM matrices are column-major. Transpose to treat m[i] as row i.
  glm::mat4 m = glm::transpose(proj);

  // Plane updates: n and d
  planes[0] = normalizePlane(m[3] + m[0]); // Left
  planes[1] = normalizePlane(m[3] - m[0]); // Right
  planes[2] = normalizePlane(m[3] + m[1]); // Bottom
  planes[3] = normalizePlane(m[3] - m[1]); // Top
  planes[4] = normalizePlane(m[3] + m[2]); // Near
  planes[5] = normalizePlane(m[3] - m[2]); // Far
}

/**
 * @brief Clear color and depth buffers
 */
void Rasterizer::clear() {
  glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/**
 * @brief Set view matrix in shader
 */
void Rasterizer::setShaderView() {
  view = camera.GetViewMatrix();
  shader->use();
  shader->setMat4("View", view);
}

/**
 * @brief Cull blocks against view frustum
 */
void Rasterizer::cullBlocks(){
  visibleCount = blocks.size();
  for (int i = 0; i < blocks.size(); i++){
    aabbIntersectsFrustum(blocks[i]);
  }
}

/**
 * @brief Test if AABB intersects or is inside the frustum
 * @param block Block to test for visibility
 */
void Rasterizer::aabbIntersectsFrustum(Block & block) {

  // Generate all 8 corners of the AABB in world space
  glm::vec3 corners[8] = {
      glm::vec3(block.bb_min.x, block.bb_min.y, block.bb_min.z),
      glm::vec3(block.bb_max.x, block.bb_min.y, block.bb_min.z),
      glm::vec3(block.bb_min.x, block.bb_max.y, block.bb_min.z),
      glm::vec3(block.bb_max.x, block.bb_max.y, block.bb_min.z),
      glm::vec3(block.bb_min.x, block.bb_min.y, block.bb_max.z),
      glm::vec3(block.bb_max.x, block.bb_min.y, block.bb_max.z),
      glm::vec3(block.bb_min.x, block.bb_max.y, block.bb_max.z),
      glm::vec3(block.bb_max.x, block.bb_max.y, block.bb_max.z)
  };

  // Transform all corners to view space and compute bounds
  glm::vec3 bb_min(FLT_MAX);
  glm::vec3 bb_max(-FLT_MAX);
  for (int i = 0; i < 8; i++) {
    glm::vec3 p = view * glm::vec4(corners[i], 1.0f);
    bb_min = glm::min(bb_min, p);
    bb_max = glm::max(bb_max, p);
  }

  // compute distance for ALL blocks (needed for sorting)
  // Note: in view space, camera looks down -Z, so frustum center is at negative Z
  glm::vec3 bb_center = 0.5f * (bb_min + bb_max);
  block.distanceToCameraCenter = glm::distance(bb_center, glm::vec3(0.0f, 0.0f, 0.0f));
  block.distanceToFrustumCenter = glm::distance(bb_center, glm::vec3(0.0f, 0.0f, -0.5f*(z_far+z_near)));

  float minDist = FLT_MAX;
  for (const Plane &plane : planes) {
    // Positive vertex: the AABB corner that maximizes dot(n, x)
    glm::vec3 p;
    p.x = (plane.n.x >= 0.0f) ? bb_max.x : bb_min.x;
    p.y = (plane.n.y >= 0.0f) ? bb_max.y : bb_min.y;
    p.z = (plane.n.z >= 0.0f) ? bb_max.z : bb_min.z;
    float dist = glm::dot(plane.n, p) + plane.d;
    minDist = std::min(dist, minDist);
  }

  // Decide classification once
  block.distanceToPlaneMin = minDist;
  block.isVisible = (minDist >= 0.0f);
  visibleCount -= !block.isVisible;
}

void Rasterizer::loadBlock(const int& blockID, const int& slotIdx, const int& count, const bool& loadToSlots) {
  dataManager.enqueueBlock(blockID, slotIdx, count, loadToSlots);
}

/**
 * @brief Load blocks for out-of-core rendering
 */
void Rasterizer::loadBlocksOOC(){

  if (!isOOC) return;

  // sort blocks: visible and near to camera center blocks first
  std::sort(blocks.begin(), blocks.end(),
            [](const Block& a, const Block& b){
              if (a.isVisible != b.isVisible) {
                return a.isVisible > b.isVisible;
              }
              if (a.isVisible) {
                return a.distanceToCameraCenter < b.distanceToCameraCenter;
              } else{
                return a.distanceToPlaneMin > b.distanceToPlaneMin;
              }
            }
          );

  // load blocks to slots
  int limit = std::min<int>(num_slots, visibleCount);
  loadBlockCount = 0;
  cacheMiss = 0;
  for (int i = 0; i < limit; i++) {
    int blockID = blocks[i].blockID;
    // see if it's already in slots
    if (updateSlotByBlockID(blockID, i)) {
      continue;
    }

    // Cache(subSlot) available. see if the block is there.
    if (isCache) {
      // swaps extracted and slots[i]
      Slot extracted;
      if (subSlots.extract(blockID, extracted)) {

        // map erase
        // int old = slots[i].blockID;
        // if (old >= 0) block_to_slot.erase(old);

        // slot <-> subSlot
        subSlots.put(std::move(slots[i]));
        slots[i] = std::move(extracted);

        // map update
        // if (slots[i].blockID >= 0) block_to_slot[slots[i].blockID] = i;
        continue;
      }
    }

    // Not found
    int count = std::min(blocks[i].count, num_points_per_slot);
    loadBlock(blockID, i, count, true);
    loadBlockCount++;
    cacheMiss++;
  }
  // std::cout << "cacheMiss: " << cacheMiss << "  " << "limit: " << limit << " " << "visibleCount: " << visibleCount << "\n";

  // initialize cache and LRU update it
  if (isCache && !cacheInitialized){
    int i = 0;
    int blockIdx = limit;
    while (i < num_subSlots && blockIdx < (int)blocks.size()) {
      int blockID = blocks[blockIdx].blockID;
      // if (isBlockInSlot(slots, blockID)) {
      //   blockIdx++;
      //   continue;
      // }
      // If it's already in subSlots, just touch it (mark as recently used)
      if (subSlots.touch(blockID)) {
        i++;
        blockIdx++;
        continue;
      }
      // not in subSlots. load it.
      int count = std::min(blocks[blockIdx].count, num_points_per_slot);
      loadBlock(blockID, i, count, false);
      loadBlockCount++;
      i++;
      blockIdx++;
    }
    cacheInitialized = true;
  }
}

/**
 * @brief Draw existing blocks already in slots. out-of-core mode.
 */
void Rasterizer::drawOldBlocksOOC()
{
  for (int i = 0; i < num_slots; i++) {
    if (slots[i].blockID != blocks[i].blockID) {
      continue;
    }
    glBindVertexArray(vao[i]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, slots[i].count * sizeof(Point), slots[i].points.data());
    glDrawArrays(GL_POINTS, 0, (GLsizei)slots[i].count);
    glBindVertexArray(0);
  }
}

/**
 * @brief Draw newly loaded blocks from worker threads. out-of-core mode.
 */
void Rasterizer::drawLoadedBlocksOOC()
{
  int count = 0;
  while (count < loadBlockCount){
    Result r;
    dataManager.getResult(r);

    if (r.loadToSlots){
      if (isCache){
        subSlots.put(std::move(slots[r.slotIdx]));
      }

      slots[r.slotIdx].blockID = r.blockID;
      slots[r.slotIdx].count = r.count;
      slots[r.slotIdx].status = LOADED;
      slots[r.slotIdx].points = std::move(r.points);

      glBindVertexArray(vao[r.slotIdx]);
      glBindBuffer(GL_ARRAY_BUFFER, vbo[r.slotIdx]);
      glBufferSubData(GL_ARRAY_BUFFER, 0,
                      slots[r.slotIdx].count * sizeof(Point),
                      slots[r.slotIdx].points.data());
      glDrawArrays(GL_POINTS, 0, (GLsizei)slots[r.slotIdx].count);
      glBindVertexArray(0);
    } else {
      Slot s;
      s.blockID = r.blockID;
      s.count = r.count;
      s.status = LOADED;
      s.points = std::move(r.points);
      subSlots.put(std::move(s));
    }
    count++;
  }
}

/**
 * @brief draws blocks in-core.
 */
void Rasterizer::drawBlocks(){
  if (isOOC) return;
  for (int i = 0; i < blocks.size(); i++){
    if (blocks[i].isVisible && blocks[i].count > 0){
      glBindVertexArray(blocks[i].vao);
      glBindBuffer(GL_ARRAY_BUFFER, blocks[i].vbo);
      glDrawArrays(GL_POINTS, 0, (GLsizei)blocks[i].count);
    }
  }
}

/**
 * @brief Swaps a specific block within vector slots
 * @param blockID: ID to search for in slots
 * @param slotIdx: The specific index in slotsTarget to perform the swap.
 */
bool Rasterizer::updateSlotByBlockID(int blockID, int targetIdx) {

  // stay with linear search.
  // auto it = block_to_slot.find(blockID); // blockID is the key
  // if (it == block_to_slot.end()) return false;

  // int srcIdx = it->second;
  // if (srcIdx == targetIdx) return true;

  // std::swap(slots[srcIdx], slots[targetIdx]);

  // // refresh map: blockID of two elements changed after swap
  // int a = slots[srcIdx].blockID;
  // int b = slots[targetIdx].blockID;

  // // a, b can be -1.
  // if (a >= 0) block_to_slot[a] = srcIdx;
  // if (b >= 0) block_to_slot[b] = targetIdx;

  // return true;

  for (int i = 0; i < slots.size(); i++) {
    if (slots[i].blockID == blockID) {
      std::swap(slots[i], slots[targetIdx]);
      return true;
    }
  }
  // block not found.
  return false;
}

/**
 * @brief returns true if the block is in slots
 *
 * @param blockID: blockID to search in slots
 * @return
 */
bool Rasterizer::isBlockInSlot(int blockID) {
  for (int i = 0; i < slots.size(); i++) {
    if (blockID == slots[i].blockID) {
      std::cout << "isBlockInSlot returns True" << std::endl;
      return true;
    }
  }
  return false;
}



/**
 * @brief Set orbital camera pose for test mode
 */
void Rasterizer::setCameraPose(){
  if (!isTest) return;
  // test mode: orbit camera around scene center
  float theta = angularSpeed * (fixedDt);
  dir = camera_position - center;
  camera_position = center + (Rz(theta) * dir);
  camera.changePose(camera_position, center);
}

/**
 * @brief Main render loop
 */
void Rasterizer::render(){

  while (!glfwWindowShouldClose(window)) {

    {
      CPU_PROFILE(profilerCPU, "Frame");

      processInput();
      setCameraPose(); // test mode
      clear();
      setShaderView();

      { CPU_PROFILE(profilerCPU, "cullBlocks"); cullBlocks(); }

      if (isOOC){
        { CPU_PROFILE(profilerCPU, "LoadOOC"); loadBlocksOOC(); }
        { CPU_PROFILE(profilerCPU, "drawOldBlocksOOC"); drawOldBlocksOOC(); }
        { CPU_PROFILE(profilerCPU, "drawLoadedBlocksOOC"); drawLoadedBlocksOOC(); }
      }else{
        { CPU_PROFILE(profilerCPU, "DrawInCore"); drawBlocks(); }
      }


      // Swap buffers & poll events
      glfwSwapBuffers(window);
      glfwPollEvents();
    }

    // export Frame mode
    // TODO: test this if this works out of the Frame scope
    exportFrame();

    // update Benchmarks
    updateBenchmarks();

    // break?
    if (profilerCPU.stats["Frame"].calls - warmup + 1 >= N) {
      glfwSetWindowShouldClose(window, true);
      break;
    }
  }

  // print stats
  printStats();
  profilerCPU.end_frame_and_print();

  // quit
  if (isOOC) dataManager.quit();
}

/**
 * @brief Process keyboard input
 */
void Rasterizer::processInput() {

  // check if ESC pressed
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

  // test mode
  if (isTest) return;

  // keys (continuous)
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.ProcessKeyboard(Camera::FORWARD, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(Camera::BACKWARD, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(Camera::LEFT, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(Camera::RIGHT, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) camera.ProcessKeyboard(Camera::UP, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) camera.ProcessKeyboard(Camera::DOWN, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) camera.ProcessKeyboard(Camera::YAW_MINUS, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) camera.ProcessKeyboard(Camera::YAW_PLUS, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) camera.ProcessKeyboard(Camera::PITCH_MINUS, fixedDt);
  if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) camera.ProcessKeyboard(Camera::PITCH_PLUS, fixedDt);

}

/**
 * @brief GLFW scroll callback
 */
void Rasterizer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

/**
 * @brief GLFW mouse callback
 */
void Rasterizer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->handleMouseMove(window, xposIn, yposIn);
}

/**
 * @brief GLFW window focus callback
 */
void Rasterizer::window_focus_callback(GLFWwindow* window, int focused)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->handleWindowFocus(window, focused);
}

/**
 * @brief Handle window focus changes
 */
void Rasterizer::handleWindowFocus(GLFWwindow* window, int focused)
{
  hasFocus = (focused == GLFW_TRUE);
  firstMouse = true;

  if (!hasFocus) return;

  double xpos = 0.0;
  double ypos = 0.0;
  glfwGetCursorPos(window, &xpos, &ypos);
  lastX = static_cast<float>(xpos);
  lastY = static_cast<float>(ypos);
}

/**
 * @brief Handle mouse movement
 */
void Rasterizer::handleMouseMove(GLFWwindow* window, double xposIn, double yposIn)
{
  if (!hasFocus) return;
  if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED) return;

  float xpos = static_cast<float>(xposIn);
  float ypos = static_cast<float>(yposIn);

  if (firstMouse) {
    lastX = xpos;
    lastY = ypos;
    firstMouse = false;
    return;
  }

  float xoffset = xpos - lastX;
  float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

  lastX = xpos;
  lastY = ypos;

  camera.ProcessMouseMovement(xoffset, yoffset);
}

/**
 * @brief GLFW framebuffer size callback
 */
void Rasterizer::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

void Rasterizer::saveFramePNG(const std::string& path, int w, int h) {
  std::vector<unsigned char> rgba(w * h * 4);

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadBuffer(GL_BACK); // capture what you're about to swap
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

  // Flip vertically (OpenGL origin is bottom-left)
  for (int y = 0; y < h / 2; ++y) {
    int top = y * w * 4;
    int bot = (h - 1 - y) * w * 4;
    for (int x = 0; x < w * 4; ++x) std::swap(rgba[top + x], rgba[bot + x]);
  }

  stbi_write_png(path.c_str(), w, h, 4, rgba.data(), w * 4);
}

/**
 * @brief Export current frame if export mode enabled
 */
void Rasterizer::exportFrame() {
  if (!isExport) return;
  char buf[256];
  std::snprintf(buf, sizeof(buf), "%s/frame_%05d.png", "../outputs", profilerCPU.stats["cullBlocks"].calls);
  saveFramePNG(buf, window_width, window_height);
}

/**
 * @brief Update benchmark statistics per frame
 */
void Rasterizer::updateBenchmarks() {
  if (profilerCPU.stats["Frame"].calls < warmup) return;
  float dt = profilerCPU.stats["Frame"].current_ms;
  float fps = (dt > 0.0f) ? (1000.0f / dt) : 0.0f;
  maxVisibleCount = std::max(maxVisibleCount, visibleCount);
  minVisibleCount = std::min(minVisibleCount, visibleCount);
  maxCacheMiss = std::max(maxCacheMiss, cacheMiss);
  minCacheMiss = std::min(minCacheMiss, cacheMiss);
  updateWindowTitle(fps);
}

/**
 * @brief Update window title with FPS info
 */
void Rasterizer::updateWindowTitle(float fps) {
  if (profilerCPU.stats["cullBlocks"].calls % 10 != 1) return;
  char buf[128];
  std::snprintf(buf, sizeof(buf),
      "MyRasterizer | FPS: %.1f | visibleCount: %d | Cache Miss: %d", fps, visibleCount, cacheMiss);
  glfwSetWindowTitle(window, buf);
}

/**
 * @brief Print benchmark results to stdout
 */
void Rasterizer::printStats() {
  auto& frameStat = profilerCPU.stats["Frame"];
  avgFPS = 1000.0f * frameStat.calls / frameStat.total_ms;
  maxFPS = 1000.0f / frameStat.min_ms;
  minFPS = 1000.0f / frameStat.max_ms;
  std::cout << "Bench N=" << N << " warmup= " << warmup << "\n";
  std::cout << "Avg FPS: " << avgFPS << "\n";
  std::cout << "Max FPS: " << maxFPS << "\n";
  std::cout << "Min FPS: " << minFPS << "\n";
  std::cout << "Max visibleCount: " << maxVisibleCount << "\n";
  std::cout << "Min visibleCount: " << minVisibleCount << "\n";
  std::cout << "Max cacheMiss: " << maxCacheMiss << "\n";
  std::cout << "Min cacheMiss: " << minCacheMiss << "\n";
}
