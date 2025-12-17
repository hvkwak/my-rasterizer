# My Rasterizer
A 3D point cloud rasterizer built with C++ and OpenGL. 

## News
- [2025-12-16] Implemented a basic 3D point cloud rasterizer with camera control. It reaches 15 FPS on the scene `Church` (67M Points) with CPU `Ryzen 7 PRO 5850U (Radeon Vega iGPU)` when tested with a 10 degrees/sec orbit camera. 

## Features
- Real-time 3D point cloud rendering from PLY file format
- Custom vertex and fragment shader support
- Interactive camera controls (orbit camera modes for testing)
- OpenGL-based rendering pipeline

## Prerequisites
- CMake 3.20 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- OpenGL 3.3 or higher
- GLFW 3.4
- GLM (OpenGL Mathematics library)

## Datasets
Compatible with 3D point cloud datasets in `.ply` format. Datasets are stored in the `data` directory. This implementation is tested with the well-known public 3D point cloud datasets (ground truth `.ply` files) from `https://www.tanksandtemples.org/`.

## Installation
```bash
sudo apt install cmake build-essential libgl1-mesa-dev libglfw3-dev libglm-dev
```

## Build
1. Clone the repository:
```bash
git clone https://github.com/hvkwak/my-rasterizer
cd my-rasterizer
```

2. Create a build directory and compile:
```bash
mkdir build
cd build
cmake ..
make
```
The executable `main` will be created in the `build` directory.

## Usage

### Basic Usage
Run the program in the `build` directory:
```bash
./main
```

This will load the default model (`../data/Barn.ply`) with default shaders.

### Command Line Arguments
You can specify custom files via command line arguments:

```bash
./main [options] [model.ply] [shader.vert] [shader.frag]
```

**Available Options:**
- `--test`: Run in test mode
- `*.ply`: Specify a PLY model file
- `*.vert`: Specify a custom vertex shader
- `*.frag`: Specify a custom fragment shader

**Example:**

```bash
./main ../data/Barn.ply ../src/shader/shader.vert ../src/shader/shader.frag
```

### Camera Controls
- **FPS Mode**: Navigate using standard FPS controls
- **Orbit Mode**: Rotate around the model

## License
[Add your license here]

## Acknowledgments
- OpenGL for the graphics API
- LearnOpenGL for the awesome guidance
- GLFW for window and input handling
- GLM for various mathematics operations
