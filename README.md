# My Rasterizer
A 3D point cloud rasterizer built with C++ and OpenGL.

## News
- (NEXT) Implemented prioritized multithreaded data streaming with a 64-slot resident cache and background prefetch of lower-priority sub-blocks.
- **[2025-12-29] Added view-dependent block culling, resulting in a measurable rendering speedup upto __ FPS.**
- **[2025-12-28] Implemented out-of-core rendering for massive point clouds exceeding available RAM.**
  - Implemented multithreaded binary data loading of 10×10×10 partitioned spatial blocks.
  - The main thread synchronizes on loading blocks to prioritize stable rendering.
  - Implemented LRU cache to reuse files during block partitioning, reducing file I/O overhead by minimizing repeated file open/close operations.
- **[2025-12-16] Implemented a basic 3D point cloud rasterizer with camera control.**
  - Achieves __ FPS on the `Church` scene (67M points, in-core) on Ryzen 7 PRO 5850U with Radeon Vega iGPU using a 10°/sec orbit camera.

## Features
- Real-time 3D point cloud rendering from PLY file format
- **Out-of-core rendering** for massive datasets exceeding available RAM
- Multi-threaded (async) data streaming with spatial partitioning
- Custom vertex and fragment shader support
- Interactive camera controls (orbit camera modes for testing)
- OpenGL-based rendering pipeline

## Prerequisites
- CMake 3.20 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- OpenGL 3.3 or higher
- GLFW 3.4
- GLM (OpenGL Mathematics library)

Install the following packages
```bash
sudo apt install cmake build-essential libgl1-mesa-dev libglfw3-dev libglm-dev
```

## Datasets
This Rasterizer is compatible with 3D point cloud datasets in `.ply` format. Datasets should be stored in the `data` directory. This implementation is tested with the well-known public 3D point cloud datasets (ground truth `.ply` files) from `https://www.tanksandtemples.org/`.

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
This will load the default model (e.g. `../data/Barn.ply`) with default shaders.

### Command Line Arguments
You can specify custom files via command line arguments:

```bash
./main [options] [model.ply] [shader.vert] [shader.frag]
```

**Available Options:**
- `--test`: Run in test mode (orbit camera pose)
- `--ooc`: Enable out-of-core rendering mode (for large datasets)
- `*.ply`: Specify a PLY model file
- `*.vert`: Specify a custom vertex shader
- `*.frag`: Specify a custom fragment shader

**Examples:**

```bash
# In-core rendering (default)
./main ../data/Barn.ply ../src/shader/shader.vert ../src/shader/shader.frag

# Out-of-core rendering for large datasets
./main --ooc ../data/Church.ply
```

### Camera Controls
- **Normal Mode**: Navigate using standard controls (Camera movements with Q, W, A, S, Z, X. Yaw/Pitch Contrtol with J, L, I, K)
- **Test Mode**: Rotate around the model with orbit camera (use argument `--test` for this mode)

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgments
- OpenGL for the graphics API
- LearnOpenGL for the awesome guidance
- GLFW for window and input handling
- GLM for various mathematics operations
