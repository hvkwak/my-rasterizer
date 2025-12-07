#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Data.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;


/**
 * @brief process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
 */
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

/**
 * @brief glfw: whenever the window size changed (by OS or user resize) this
 * callback function executes
 */
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina displays.
  glViewport(0, 0, width, height);
}

int main(){

  // glfw: initialize and configure
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // glfw window creation
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MyRasterizer", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // glad: load all OpenGL function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  // build and compile our shader program
  Shader shader("../src/shader/shader.vert", "../src/shader/shader.frag");

  // Replace with your actual file path
  std::vector<Point> points = readPLY("../data/Barn.ply");

  // VBO, VAO
  unsigned int pointVBO, pointVAO;
  glGenVertexArrays(1, &pointVAO);
  glGenBuffers(1, &pointVBO);
  glBindVertexArray(pointVAO);
  glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
  glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(Point), points.data(), GL_STATIC_DRAW);

  // position attribute
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // texture coord attribute
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  // render loop
  // while (!glfwWindowShouldClose(window)){

  //   // input
  //   processInput(window);

  //   // render

  // }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
  return 0;
}
