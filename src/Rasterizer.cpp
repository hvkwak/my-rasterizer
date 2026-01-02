//=============================================================================
//
//   Rasterizer - Main rendering engine for point cloud visualization
//
//   Copyright (C) 2026 Hyovin Kwak
//
//=============================================================================

#include "Rasterizer.h"
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
                      bool isSlot_,
                      bool isCache_,
                      unsigned int window_width_,
                      unsigned int window_height_,
                      float z_near_,
                      float z_far_,
                      float rotateAngle_,
                      float distanceFactor_){
  // Set member variables
  plyPath = plyPath_;
  outDir = outDir_;
  shader_vert = shader_vert_;
  shader_frag = shader_frag_;
  isTest = isTest_;
  isOOC = isOOC_;
  window_width = window_width_;
  window_height = window_height_;
  z_near = z_near_;
  z_far = z_far_;
  angularSpeed = glm::radians(rotateAngle_);
  distFactor = distanceFactor_; // for orbital camera pose in test mode
  blocks.resize(NUM_BLOCKS);

  // setups
  if (!setupWindow()) return false;
  if (!setupDataManager()) return false;
  if (!filterBlocks()) return false;
  if (!setupRasterizer()) return false;
  if (!setupCameraPose()) return false;
  if (!setupCallbacks()) return false;
  if (!setupShader()) return false;
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
  if(!dataManager.init(plyPath, outDir, isOOC, bb_min, bb_max, blocks)){
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
  return true;
}

/**
 * @brief Setup buffer wrapper for in-core or out-of-core rendering
 * @return true if buffer setup succeeds
 */
bool Rasterizer::setupBufferWrapper(){
  return isOOC ? setupSlots() : setupBufferPerBlock();
}

// /**
//  * @brief sets up gl Buffers in non out-of-core mode.
//  * @return true, if setup is successful
//  */
// bool Rasterizer::setupBuffer(){
//   glGenVertexArrays(1, &VAO);
//   glGenBuffers(1, &VBO);

//   glBindVertexArray(VAO);
//   glBindBuffer(GL_ARRAY_BUFFER, VBO);
//   glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(Point), points.data(), GL_STATIC_DRAW);

//   // Attribute 0: Position
//   // location, size, type, stride, offset
//   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
//   glEnableVertexAttribArray(0);

//   // Attribute 1: Color
//   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)(sizeof(glm::vec3)));
//   glEnableVertexAttribArray(1);
//   return true;
// }

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
  slots.resize(NUM_SLOTS);
  subSlots.resize(NUM_SUB_SLOTS);

  for (int i = 0; i < NUM_SLOTS; ++i) {
    glGenVertexArrays(1, &slots[i].vao);
    glGenBuffers(1, &slots[i].vbo);

    glBindVertexArray(slots[i].vao);
    glBindBuffer(GL_ARRAY_BUFFER, slots[i].vbo);

    // set the capacity per slot, we use glBufferSubData
    glBufferData(GL_ARRAY_BUFFER,
                 NUM_POINTS_PER_SLOT * sizeof(Point),
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
 * @brief Cull blocks against view frustum
 */
void Rasterizer::cullBlocks(){
  visibleBlocks = blocks.size();
  for (int i = 0; i < blocks.size(); i++){
    aabbIntersectsFrustum(blocks[i]);
  }
}

/**
 * @brief Test if AABB intersects or is inside the frustum
 * @param block Block to test for visibility
 */
void Rasterizer::aabbIntersectsFrustum(Block & block) {

  block.isVisible = true;

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

  for (const Plane &plane : planes) {
    // Positive vertex: the AABB corner that maximizes dot(n, x)
    glm::vec3 p;
    p.x = (plane.n.x >= 0.0f) ? bb_max.x : bb_min.x;
    p.y = (plane.n.y >= 0.0f) ? bb_max.y : bb_min.y;
    p.z = (plane.n.z >= 0.0f) ? bb_max.z : bb_min.z;

    // If even the best-case vertex is outside, the whole box is outside
    if (glm::dot(plane.n, p) + plane.d < 0.0f) {
      block.isVisible = false;
      visibleBlocks--;
      return;
    }
  }
  // add distance to camera center for visible blocks
  glm::vec3 bb_center = 0.5f * (bb_min + bb_max);
  block.distance = glm::distance(glm::vec3(0.0f, 0.0f, 0.0f), bb_center);
}


/**
 * @brief Load blocks for out-of-core rendering
 */
void Rasterizer::loadBlocksOOC(){

  // sort blocks: visible and near to camera center blocks first
  std::sort(blocks.begin(), blocks.end(),
            [](Block& a, Block& b){
              return a.isVisible != b.isVisible ? a.isVisible > b.isVisible : a.distance < b.distance;
            }
          );

  // load to slots
  const int limit = std::min<int>(NUM_SLOTS, blocks.size());
  loadedBlocks = 0; // loaded visible blocks.
  loadingBlocks = 0;
  cachedBlocks = 0;
  for (int i = 0; i < limit; i++) {
    bool isVisible = blocks[i].isVisible;
    if (isVisible){
      int blockID = blocks[i].blockID;
      int slotIdx = findSlot(blockID); // position in slots
      int blockCount = std::min(blocks[i].count, NUM_POINTS_PER_SLOT);

      if (slotIdx == -1) {
        // not available in slot. load.
        dataManager.enqueueBlock(blockID, i, blockCount);
        slots[i].blockID = blockID;
        slots[i].status = LOADING;
        loadingBlocks++;
      } else {
        // available in slot. just swap it. slots[i] now has the right block
        std::swap(slots[slotIdx], slots[i]);
        slots[i].status = LOADED;
        loadedBlocks++;
      }
    } else {
      slots[i].blockID = -1; //
      slots[i].status = EMPTY;
    }
  }
  // std::cout << "loadedBlocks this frame: " << loadedBlocks << "\n";

  // caching additional blocks
  for (int i = 0; i < NUM_SUB_SLOTS; i++){
    int blockID = blocks[limit + i].blockID;
    int blockCount = std::min(blocks[i].count, NUM_POINTS_PER_SLOT);
    dataManager.enqueueBlock(blockID, i, blockCount);

  }
}

/**
 * @brief Draw blocks in out-of-core mode
 */
void Rasterizer::drawBlocksOOC()
{
  // draw existing slots first.
  int i = 0;
  int count = 0;
  while (count < loadedBlocks) {
    if (slots[i].status != LOADED) {
      i++;
      continue;
    };
    // bind current block VAO, VBO
    glBindVertexArray(slots[i].vao);
    glBindBuffer(GL_ARRAY_BUFFER, slots[i].vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, slots[i].count * sizeof(Point), slots[i].points.data());

    // Draw points
    glDrawArrays(GL_POINTS, 0, (GLsizei)slots[i].count);
    glBindVertexArray(0);
    count++;
    i++;
  }

  // draw newly loaded blocks: better synchronization when this goes next.
  count = 0;
  while (count < loadingBlocks){
    Result r;
    dataManager.getResult(r);

    // update slots[r.slotIdx]
    slots[r.slotIdx].blockID = r.blockID;
    slots[r.slotIdx].count = r.count;
    slots[r.slotIdx].status = LOADED;
    slots[r.slotIdx].points = std::move(r.points);

    // bind current block VAO, VBO
    glBindVertexArray(slots[r.slotIdx].vao);
    glBindBuffer(GL_ARRAY_BUFFER, slots[r.slotIdx].vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, slots[r.slotIdx].count * sizeof(Point), slots[r.slotIdx].points.data());

    // Draw points
    glDrawArrays(GL_POINTS, 0, (GLsizei)slots[r.slotIdx].count);

    glBindVertexArray(0);
    count++;
  }
}

/**
 * @brief draws blocks in-core.
 */
void Rasterizer::drawBlocks(){
  for (int i = 0; i < blocks.size(); i++){
    if (blocks[i].isVisible && blocks[i].count > 0){
      glBindVertexArray(blocks[i].vao);
      glBindBuffer(GL_ARRAY_BUFFER, blocks[i].vbo);
      glDrawArrays(GL_POINTS, 0, (GLsizei)blocks[i].count);
    }
  }
}

/**
 * @brief Find slot index containing given block ID
 * @param blockID Block ID to search for
 * @return Slot index, or -1 if not found
 */
int Rasterizer::findSlot(int blockID) {
  for (int i = 0; i < NUM_SLOTS; i++){
    if (blockID == slots[i].blockID){
      return i;
    }
  }
  return -1;
}


/**
 * @brief Set orbital camera pose for test mode
 */
void Rasterizer::setOrbitCamera(){
  // test mode: orbit camera around scene center
  float theta = angularSpeed * deltaTime;
  glm::vec3 dir = camera_position - center;
  camera_position = center + (Rz(theta) * dir);
  camera.changePose(camera_position, center);
}

/**
 * @brief Main render loop
 */
void Rasterizer::render(){

  while (!glfwWindowShouldClose(window)) {

    updateInfo();
    processInput();

    if (isTest) setOrbitCamera();

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();

    view = camera.GetViewMatrix();
    shader->setMat4("View", view);

    // Culling
    cullBlocks();

    // load and draw blocks
    if (isOOC) {

      // Out-of-core
      loadBlocksOOC();
      drawBlocksOOC();

    } else {

      // In-Core. Points are already there.
      drawBlocks();

    }

    // Swap buffers & poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // quit
  if (isOOC) dataManager.quit();
  std::cout << "Max FPS: " << maxFPS << std::endl;
  std::cout << "Min FPS: " << minFPS << std::endl;
  std::cout << "Max visibleBlocks: " << maxVisibleBlocks << std::endl;
  std::cout << "Min visibleBlocks: " << minVisibleBlocks << std::endl;
}


/**
 * @brief Update FPS and frame statistics
 */
void Rasterizer::updateInfo(){
  float fps = 0.0f;
  float currentFrame = static_cast<float>(glfwGetTime());
  if (!isFPSInitialized){
    lastFrame = currentFrame;
    isFPSInitialized = true;
    return;
  }
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  acc += deltaTime;
  frames++;

  if (acc > 1.0){
    fps = frames / acc;

    // benchmark FPS, visibleBlocks
    if (fps > maxFPS) maxFPS = fps;
    if (fps < minFPS) minFPS = fps;
    if (visibleBlocks > maxVisibleBlocks) maxVisibleBlocks = visibleBlocks;
    if (visibleBlocks < minVisibleBlocks) minVisibleBlocks = visibleBlocks;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "MyRasterizer | FPS: %.1f | visibleBlocks: %d", fps, visibleBlocks);
    glfwSetWindowTitle(window, buf);
    acc = 0.0f;
    frames = 0;
  }
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
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(Camera::BACKWARD, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(Camera::LEFT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(Camera::RIGHT, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) camera.ProcessKeyboard(Camera::UP, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) camera.ProcessKeyboard(Camera::DOWN, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) camera.ProcessKeyboard(Camera::YAW_MINUS, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) camera.ProcessKeyboard(Camera::YAW_PLUS, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) camera.ProcessKeyboard(Camera::PITCH_MINUS, deltaTime);
  if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) camera.ProcessKeyboard(Camera::PITCH_PLUS, deltaTime);

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


// void uploadToSlot(Slot& s, BlockID id, const std::vector<Point>& points, uint64_t frame) {
//   const uint32_t uploadCount = (uint32_t)std::min<size_t>(points.size(), CAP_POINTS_PER_SLOT);

//   glBindBuffer(GL_ARRAY_BUFFER, s.vbo);

//   // ✅ 내용만 덮어쓰기 (버퍼 크기 안 바뀜!)
//   glBufferSubData(GL_ARRAY_BUFFER,
//                   0,
//                   uploadCount * sizeof(Point),
//                   points.data());

//   glBindBuffer(GL_ARRAY_BUFFER, 0);

//   s.block = id;
//   s.count = uploadCount;
//   s.lastUsedFrame = frame;
// }
