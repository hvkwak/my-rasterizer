cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0072 NEW)

# Name of the project
project(main LANGUAGES C CXX)

# Use C++17
set(OpenGL_GL_PREFERENCE GLVND)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# build type
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")


# ---- Source files ----
# We build one executable that contains:
#  - your main.cpp
#  - glad.c (OpenGL loader)
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/glad.c
    src/DataManager.cpp
    src/Rasterizer.cpp
    src/Camera.cpp
    src/Cache.cpp
    src/Plane.cpp
)

# ---- Include directories ----
# Tell the compiler where to find:
#  - glad headers (include/glad, include/KHR)
#  - shader_s.h, stb_image.h (if you put them in include/)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

# ---- find packages: OpenGL, GLFW, .. ----
# The system OpenGL library (libGL.so on Linux)
find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)
message(STATUS "GLM included at ${GLM_INCLUDE_DIR}")

# Threading support for Queue class
find_package(Threads REQUIRED)



##############################################################################
# STB
# (place *before* GLFW since GLFW has an old copy of stb_image_write.h)
##############################################################################
set(STB_SOURCE_DIR "external/stb_image")
include_directories(${STB_SOURCE_DIR})
add_subdirectory(${STB_SOURCE_DIR})


# GLFW is the window / context / input library.
# On Ubuntu, install it with:
#   sudo apt install libglfw3-dev
#
# and use find_package:
#   find_package(glfw3 3.4 REQUIRED)
# or take what's built from source:
set(GLFW_SRC_DIR "/home/hyobin/glfw-3.4")
add_subdirectory(${GLFW_SRC_DIR} ${CMAKE_BINARY_DIR}/glfw-build)
# ---- find packages: OpenGL, GLFW, .. ends here----

# ---- Link libraries ----
# Link our executable with:
#  - OpenGL::GL (the OpenGL library)
#  - glfw (the GLFW library, which also pulls in X11 stuff on Linux)
#  - Threads::Threads (pthread for Queue class)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        OpenGL::GL
        glfw
        Threads::Threads
        stb_image
)

# ---- Notes ----
# - glm is header-only; if you installed libglm-dev on Ubuntu, CMake
#   will find it automatically through /usr/include, so no extra linking
#   is needed here.
# - stb_image.h is also header-only; just make sure it is in "include/"
#   or a system include path.
# - "Shader" and "FileSystem" from LearnOpenGL are usually supplied as
#   shader_s.h and filesystem.h; put them in "include/" as well.
