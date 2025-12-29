#include "Rasterizer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "DataManager.h"
#include "Utils.h"
#include "Shader.h"
#include <iostream>
#include <cstdio>
#include <cfloat>
#include <limits>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Rasterizer::Rasterizer(){}

Rasterizer::~Rasterizer(){
  if (shader != nullptr){
    if (glInitialized && shader->ID != 0) glDeleteProgram(shader->ID);
    delete shader;
  }
  // optional: de-allocate all resources once they've outlived their purpose:
  // ------------------------------------------------------------------------
  if (glInitialized) {
    if (VAO != 0) glDeleteVertexArrays(1, &VAO);
    if (VBO != 0) glDeleteBuffers(1, &VBO);
  }

  if (window != nullptr) {
    glfwDestroyWindow(window);
    window = nullptr;
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
}

bool Rasterizer::init(const std::filesystem::path& plyPath_,
                      const std::filesystem::path& outDir_,
                      const std::filesystem::path& shader_vert_,
                      const std::filesystem::path& shader_frag_,
                      bool isTest_,
                      bool isOOC_,
                      unsigned int window_width_,
                      unsigned int window_height_,
                      float z_near_,
                      float z_far_,
                      float rotateAngle_){
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
  blocks.resize(NUM_BLOCKS);

  // setups
  if (!setupWindow()) return false;
  if (!setupDataManager()) return false;
  if (!setupRasterizer()) return false;
  if (!setupCameraPose()) return false;
  if (!setupCallbacks()) return false;
  if (!setupShader()) return false;
  if (!setupCulling()) return false;

  // setup Buffers
  if (isOOC) {
    if (!setupBufferPerBlock()) return false;
  } else {
    if (!setupBuffer()) return false;
  }
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
 * @brief initialize Camera Position based on bb_min, bb_max for better visualization
 *
 * @param
 * @return
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

bool Rasterizer::setupCulling(){
  buildFrustumPlanes();
  return true;
}


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
 * @brief setup input and callbacks
 *
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


bool Rasterizer::setupBufferWrapper(){
  return isOOC ? setupBufferPerBlock() : setupBuffer();
}

/**
 * @brief sets up gl Buffers in non out-of-core mode.
 * @return true, if setup is successful
 */
bool Rasterizer::setupBuffer(){
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(Point), points.data(), GL_STATIC_DRAW);

  // Attribute 0: Position
  // location, size, type, stride, offset
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
  glEnableVertexAttribArray(0);

  // Attribute 1: Color
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)(sizeof(glm::vec3)));
  glEnableVertexAttribArray(1);
  return true;
}

/**
 * @brief sets up gl Buffers in out-of-core mode.
 * @return true, if setup is successful
 */
bool Rasterizer::setupBufferPerBlock(){

  for (int id = 0; id < NUM_BLOCKS; id++) {
    glGenVertexArrays(1, &blocks[id].vao);
    glGenBuffers(1, &blocks[id].vbo);

    glBindVertexArray(blocks[id].vao);
    glBindBuffer(GL_ARRAY_BUFFER, blocks[id].vbo);

    // attrib setup once per VAO
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)(sizeof(glm::vec3))); // adjust field name
    glEnableVertexAttribArray(1);
  }
  return true;
}

// Extract planes from OpenGL-style clip space VP
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

void Rasterizer::cullBlocks(){
  for (int i = 0; i < NUM_BLOCKS; i++){
    aabbIntersectsFrustum(blocks[i]);
  }
}

// Returns true if AABB intersects or is inside the frustum
void Rasterizer::aabbIntersectsFrustum(Block & block) {

  if (block.count == 0) return;
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
      return;
    }
  }

  // // transform it to view space
  // glm::vec3 p1 = view*glm::vec4(block.bb_min, 1.0f);
  // glm::vec3 p2 = view*glm::vec4(block.bb_max, 1.0f);
  // glm::vec3 bb_min = glm::min(p1, p2);
  // glm::vec3 bb_max = glm::max(p1, p2);

  // for (const Plane &pl : planes) {
  //   // Positive vertex: the AABB corner that maximizes dot(n, x)
  //   glm::vec3 p;
  //   p.x = (pl.n.x >= 0.0f) ? bb_max.x : bb_min.x;
  //   p.y = (pl.n.y >= 0.0f) ? bb_max.y : bb_min.y;
  //   p.z = (pl.n.z >= 0.0f) ? bb_max.z : bb_min.z;

  //   // If even the best-case vertex is outside, the whole box is outside
  //   if (glm::dot(pl.n, p) + pl.d < 0.0f){
  //     block.isVisible = false;
  //     return;
  //   }
  // }
}

void Rasterizer::drawBlocks()
{
  constexpr size_t MAX_UPLOAD = 1000; // TODO: delete this after prototyping.

  for (int i = 0; i < loadedBlocks; ++i) {

    Result r;
    dataManager.getResult(r);

    // uploadCount update
    // const size_t uploadCount = std::min(r.points.size(), MAX_UPLOAD);
    const size_t uploadCount = blocks[r.blockID].count;

    if (uploadCount == 0) {
      continue;
    }

    // bind current block VAO, VBO
    glBindVertexArray(blocks[r.blockID].vao);
    glBindBuffer(GL_ARRAY_BUFFER, blocks[r.blockID].vbo);

    // takes first "uploadCount" points
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(uploadCount * sizeof(Point)), r.points.data(), GL_STREAM_DRAW);

    // blocks[r.blockID].count = (int)uploadCount;
    glDrawArrays(GL_POINTS, 0, (GLsizei)uploadCount);

  }
  // std::cout << "loadedBlocks this frame: " << loadedBlocks << "\n";
  glBindVertexArray(0);
}


void Rasterizer::loadBlocks(){
  // load Blocks
  loadedBlocks = 0;
  for (int i = 0; i < NUM_BLOCKS; i++) {
    Block & block = blocks[i];
    if (block.isVisible && block.count > 0){
      dataManager.loadBlock(i, block.count); // TODO: Camera parameter should consider culling + count here
      loadedBlocks++;
    }
  }
};

void Rasterizer::render(){

  while (!glfwWindowShouldClose(window)) {

    updateFPS();
    processInput();

    if (isTest){
      // test mode: orbit camera around scene center
      const float theta = angularSpeed * deltaTime;

      const glm::vec3 dir = camera_position - center;
      camera_position = center + (Rz(theta) * dir);
      camera.changePose(camera_position, center);
    }

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader->use();

    view = camera.GetViewMatrix();
    shader->setMat4("View", view);

    if (isOOC) {

      // Culling
      cullBlocks();

      // load of blocks out-of-core
      loadBlocks();

      // get results and draw
      drawBlocks();

    } else {
      // TODO: in-core rendering should be available
      // Bind VAO containing our point cloud
      glBindVertexArray(VAO);

      // Draw N points
      glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));
    }

    // Swap buffers & poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  dataManager.quit();
}


void Rasterizer::updateFPS(){
  float currentFrame = static_cast<float>(glfwGetTime());
  deltaTime = currentFrame - lastFrame;
  lastFrame = currentFrame;

  acc += deltaTime;
  frames++;

  if (acc > 1.0){
    float fps = frames / acc;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "MyRasterizer | FPS: %.1f", fps);
    glfwSetWindowTitle(window, buf);

    acc = 0.0f;
    frames = 0;
  }
}


/**
 * @brief query GLFW whether relevant keys are pressed/released this frame and react accordingly
 *        -> every frame to check continuous input states
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

void Rasterizer::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void Rasterizer::mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->handleMouseMove(window, xposIn, yposIn);
}

void Rasterizer::window_focus_callback(GLFWwindow* window, int focused)
{
  auto* self = static_cast<Rasterizer*>(glfwGetWindowUserPointer(window));
  if (self == nullptr) return;
  self->handleWindowFocus(window, focused);
}

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
 * @brief glfw: whenever the window size changed (by OS or user resize) this
 * callback function executes
 */
void Rasterizer::framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}
