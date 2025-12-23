#include "Rasterizer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "DataHandler.h"
#include "Utils.h"
#include "Shader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
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

bool Rasterizer::init(const std::string& plyPath,
                      const std::string& outDir,
                      const std::string& shader_vert,
                      const std::string& shader_frag,
                      bool isTest_,
                      bool isOOC_,
                      unsigned int window_width_,
                      unsigned int window_height_,
                      float z_near_,
                      float z_far_,
                      float rotateAngle_){
  // Set member variables
  isTest = isTest_;
  isOOC = isOOC_;
  window_width = window_width_;
  window_height = window_height_;
  z_near = z_near_;
  z_far = z_far_;
  angularSpeed = glm::radians(rotateAngle_);

  // setups
  if (!setupWindow()) return false;
  if (!setupData(plyPath, outDir)) return false; // runs before setupRasterizer().
  if (!setupRasterizer()) return false; // runs before CameraPose()
  if (!setupCameraPose()) return false;
  if (!setupInput()) return false;
  if (!setupShader(shader_vert, shader_frag)) return false;
  if (!setupBuffer()) return false;
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

bool Rasterizer::setupShader(const std::string& shader_vert, const std::string& shader_frag){
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
  glm::mat4 model = glm::mat4(1.0f); // identity for now
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)window_width / (float)window_height, z_near, z_far);
  shader->setMat4("Model", model);
  shader->setMat4("Proj", proj);
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
bool Rasterizer::setupInput(){
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

bool Rasterizer::setupData(const std::string& plyPath, const std::string& outDir){

  // Reset bounding box to initial state
  bb_min = glm::vec3(std::numeric_limits<float>::max());
  bb_max = glm::vec3(std::numeric_limits<float>::lowest());

  if (isOOC){
    // initialize Out-of-core mode
    if (!dataHandler.init(outDir)){
      std::cerr << "Error: DataHandler.init(). Exiting." << std::endl;
      return false;
    }
    if (!dataHandler.createBlocks(plyPath, bb_min, bb_max)){
      std::cerr << "Error: DataHandler.createBlocks(). Exiting." << std::endl;
      return false;
    }
    // if (!dataHandler.loadPoints()){
    //   std::cerr << "Error: DataHandler.createBlocks(). Exiting." << std::endl;
    //   return false;
    // }
  } else {
    // keep all points in one std::vector.
    points = dataHandler.readPLY(plyPath, bb_min, bb_max);
  }

  // check if all points are loaded
  if (points.empty()) {
    std::cerr
        << "Error: Could not load PLY file or file was corrupted. Exiting."
        << std::endl;
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
 * @brief sets up gl Buffers in non out-of-core mode.
 * @return true, if setup is successful
 */
bool Rasterizer::setupBuffer(){
  if (!isOOC && points.empty()) return false;

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
bool Rasterizer::setupBufferBlocks(){


  return true;
}

/**
 * @brief sets up gl Buffers in out-of-core mode by loading all 1000 block files.
 * @param outDir directory containing block_XXXX.bin files
 * @return true, if setup is successful
 */
bool Rasterizer::setupBufferVer2(const std::string& outDir){
  if (!isOOC) {
    std::cerr << "Error: setupBufferVer2() should only be called in OOC mode." << std::endl;
    return false;
  }

  points.clear();
  points.reserve(10000000); // Reserve space for approx 10M points (adjust as needed)

  // Read all 1000 block files
  for (int id = 0; id < BLOCKS; ++id) {
    char name[64];
    std::snprintf(name, sizeof(name), "block_%04d.bin", id);
    std::string filePath = (std::filesystem::path(outDir) / name).string();

    std::ifstream infile(filePath, std::ios::binary);
    if (!infile.is_open()) {
      std::cerr << "Warning: Could not open block file: " << filePath << std::endl;
      continue; // Skip missing blocks
    }

    // Read count
    uint32_t count = 0;
    infile.read(reinterpret_cast<char*>(&count), sizeof(uint32_t));
    if (count == 0) {
      infile.close();
      continue; // Skip empty blocks
    }

    // Read all PointOOC structs
    std::vector<PointOOC> blockPoints(count);
    infile.read(reinterpret_cast<char*>(blockPoints.data()), count * sizeof(PointOOC));

    if ((uint32_t)infile.gcount() != count * sizeof(PointOOC)) {
      std::cerr << "Error: Failed to read complete data from: " << filePath << std::endl;
      infile.close();
      return false;
    }
    infile.close();

    // Convert PointOOC -> Point
    for (const auto& pooc : blockPoints) {
      Point p;
      p.pos = glm::vec3(pooc.x, pooc.y, pooc.z);
      p.color = glm::vec3(
        static_cast<float>(pooc.r) / 255.0f,
        static_cast<float>(pooc.g) / 255.0f,
        static_cast<float>(pooc.b) / 255.0f
      );
      points.push_back(p);
    }
  }

  if (points.empty()) {
    std::cerr << "Error: No points loaded from block files." << std::endl;
    return false;
  }

  std::cout << "Loaded " << points.size() << " points from " << BLOCKS << " block files." << std::endl;

  // Now upload to GPU
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_STATIC_DRAW);

  // Attribute 0: Position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
  glEnableVertexAttribArray(0);

  // Attribute 1: Color
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)(sizeof(glm::vec3)));
  glEnableVertexAttribArray(1);

  return true;
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

    glm::mat4 view = camera.GetViewMatrix();
    shader->setMat4("View", view);

    // Bind VAO containing our point cloud
    glBindVertexArray(VAO);

    // Draw N points
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));

    // Swap buffers & poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
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
