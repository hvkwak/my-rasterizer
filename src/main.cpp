#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "Data.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

// settings
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;


/**
 * @brief process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
 */
void processInput(GLFWwindow *window) {
    // We check this every single frame
    // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    //     camera.ProcessKeyboard(FORWARD, deltaTime);
    // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    //     camera.ProcessKeyboard(BACKWARD, deltaTime);
}


/**
 * @brief sets key callback
 */
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // We only check this when an event happens
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // optional: more functions
    // if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    //     toggleWireframeMode();
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
  GLFWwindow *window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "MyRasterizer", NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);

  // Setting Callbacks
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetKeyCallback(window, key_callback); // Hook up the callback

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
  unsigned int VBO, VAO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  glBindVertexArray(VAO);

  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(Point), points.data(), GL_STATIC_DRAW);

  // position attribute
  // --- Attribute 0: Position ---
  // location = 0
  // size     = 3 (x, y, z)
  // type     = GL_FLOAT
  // stride   = sizeof(Point) (The size of the whole struct)
  // offset   = 0 (or offsetof(Point, pos))
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)0);
  glEnableVertexAttribArray(0);

  // --- Attribute 1: Color ---
  // location = 1
  // size     = 3 (r, g, b)
  // type     = GL_FLOAT
  // stride   = sizeof(Point)
  // offset   = sizeof(glm::vec3) (It starts after the position data)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Point), (void *)(sizeof(glm::vec3)));
  glEnableVertexAttribArray(1);

  glm::mat4 model = glm::mat4(1.0f); // identity for now
  glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), // camera position
                               glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
                               glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
  );
  glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 1.0f, 1000.0f);

  shader.use();
  shader.setMat4("uModel", model);
  shader.setMat4("uView", view);
  shader.setMat4("uProj", proj);

  while (!glfwWindowShouldClose(window)) {

    processInput(window);

    // 1. Set viewport & clear
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 2. Use shader program
    // glUseProgram(program);

    // (optional) update view/proj if camera moves
    // glUniformMatrix4fv(...);

    // 3. Bind VAO containing our point cloud
    glBindVertexArray(VAO);

    // 4. Draw N points
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(points.size()));

    glBindVertexArray(0);

    // 5. Swap buffers & poll events
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // glfw: terminate, clearing all previously allocated GLFW resources.
  glfwTerminate();
  return 0;
}
