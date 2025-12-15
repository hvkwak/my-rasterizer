#include "Rasterizer.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Data.h"
#include "utils.h"
#include "Shader.h"
#include <iostream>
#include <cstdio>
#include <cfloat>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Rasterizer::Rasterizer(bool isTest_): isTest(isTest_) {}


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

bool Rasterizer::init(const std::string& read_filename, const std::string& read_shader_vert, const std::string& read_shader_frag){
  if (!setupWindow()) return false;
  if (!setupData(read_filename)) return false; // runs before setupRasterizer().
  if (!setupRasterizer()) return false; // runs before CameraPose()
  if (!setupCameraPose()) return false;
  if (!setupInput()) return false;
  if (!setupShader(read_shader_vert, read_shader_frag)) return false;
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
  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glEnable(GL_DEPTH_TEST);
  glPointSize(3.0f);
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
  camera_position = center + glm::vec3(0.5f * diag, 0.7f * diag, 1.0f * diag);
  camera.changePose(camera_position, center);
  return true;
}

bool Rasterizer::setupShader(const std::string& read_shader_vert, const std::string& read_shader_frag){
  // build and compile our shader program
  try {
    shader = new Shader(read_shader_vert.c_str(), read_shader_frag.c_str()); // keep it like this :(
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
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, z_near, z_far);
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
  window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MyRasterizer", NULL, NULL);
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

bool Rasterizer::setupData(const std::string& read_filename){
  // Replace with your actual file path to setup vertex data
  bb_min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
  bb_max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
  points = readPLY(read_filename, bb_min, bb_max);

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
            << std::endl;
  std::cout << "bb_max: "
            << bb_max.x << ", "
            << bb_max.y << ", "
            << bb_max.z << ", "
            << std::endl;

  return true;
}

bool Rasterizer::setupBuffer(){
  if (points.empty()) return false;

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
      // test mode: orbit camera around scene center (time-based, radians)
      const float angularSpeed = glm::radians(10.0f); // rad/sec
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
