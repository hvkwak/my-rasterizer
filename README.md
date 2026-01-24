# My Rasterizer
An experimental out-of-core 3D point cloud rasterizer for interactive visualization of massive datasets, featuring block-based streaming, slot-based residency, view-dependent culling, and comparative benchmarking of in-core and out-of-core rendering. Built with C++ and OpenGL.

## Features
- **Real-time 3D point cloud rasterization** using an OpenGL-based rendering pipeline with interactive camera control
- **Out-of-core rendering architecture** for massive point clouds exceeding available RAM
  - Multi-threaded, block-based data streaming
  - Slot-based block residency to reduce per-frame buffer reallocation overhead
  - Caching (LRU) of lower-priority subslots to extend streaming efficiency
- **Spatial partitioning and view-dependent block culling** for both in-core and out-of-core rendering
- **LRU-based file streaming cache** to minimize repeated file open/close operations during block partitioning
- **Comparative benchmarking framework**
  - In-core and out-of-core rendering strategies
  - Identical camera motion and controlled block capacity constraints
- **Offline frame export** for qualitative evaluation and GIF generation
- **CPU profiler** for finding bottlenecks
- Custom vertex and fragment shader support


## News
- [2026-01-24] Implemented cpu profiler. Bug fixes and new benchmarks!
- [2026-01-21] Attempted `std::unordered_map` for slot lookup optimization in `updateSlotByBlockID()`. Hash map overheads outweigh benefit for small slot counts. Reverted to linear search.
- [2026-01-14] Fixed bugs causing excessive cache misses! Tested several sort/caching strategies. Implemented LRU caching (`SubslotsCache`) for subslots.
- [2026-01-04] Added PNG frame export to support GIF generation
- [2026-01-04] Added benchmarks that compare in-core rendering with multiple out-of-core strategies under identical camera motion and block capacity constraints.
- [2026-01-03] Implemented custom caching of lower-priority subslots to extend multi-threaded data streaming.
- [2026-01-01] Implemented out-of-core rendering based on reserved block slots, reducing `BufferData()` overhead for every block per frame.
- [2025-12-30] Implemented in-core rendering with view-dependent block culling.
- [2025-12-29] Implemented view-dependent block culling.
- [2025-12-28] Implemented out-of-core rendering for massive point clouds exceeding available RAM.
  - Implemented spatial block partitioning with multi-threaded file I/O.
  - Implemented LRU cache to reuse files (`FileStreamCache`) during block partitioning, reducing file I/O overhead by minimizing repeated file open/close operations.
- [2025-12-16] Implemented a basic 3D point cloud rasterizer with camera control.

## Benchmarks
This project is tested with the `Church` scene (67M points) from [Tanks and Temples](https://www.tanksandtemples.org/) on Ryzen 7 PRO 5850U with Radeon Vega iGPU using a 30°/sec (with `fixedDt` = 1.0/60.0) orbital camera poses. Filtering empty blocks reduces blocks from 1000 (10x10x10) to 514 blocks. Max/Min visible blocks: 198 / 90. For Out-of-core rendering, a subset of visible blocks is "loaded" into available slots for rendering. Test #2 and #3 are configured with 154 slots (30% of non-empty blocks). Maximum capacity per slot is 130K points (≈ 67M points / 514 blocks). 5 Workers were enabled for out-of-core multi-threaded data streaming. For fair comparison, in-core rendering uses the same block and point counts per frame as out-of-core.

| Nr. | slots | subslots | FPS Avg / Max / Min | cacheMiss Max / Min | Config                 | Notes                                                   |
| :-: | :---: | :------: | :-----------: | :-----------------: | :--------------------- | :------------------------------------------------------ |
|  #1 |   154 |        — | 65.15 / 180.19 / 21.40 |                   — | `--test`               | In-core (baseline) (Baseline)                                     |
|  #2 |   154 |        — | 62.02 / 163.11 / 6.14 |               4 / 0 | `--ooc --test`         | Out-of-core, no cache      |
|  #3 |   154 |       77 | 43.32 / 314.34 / 5.73 |               4 / 0 | `--ooc --test --cache` | Out-of-core + LRU cache   |

## Outputs
Side-by-side GIFs of `Church` rasterized with identical camera poses: **Original** (left) and **Out-of-core** (right).
<table>
  <tr>
    <th colspan="2" align="left">Church Scene Rasterization</th>
  </tr>
  <tr>
    <td align="center">
      <a href="outputs/church.gif">
        <img src="outputs/church.gif" alt="Original" width="360">
      </a><br>
      <sub>Original</sub>
    </td>
    <td align="center">
      <a href="outputs/church_OOC.gif">
        <img src="outputs/church_OOC.gif" alt="Out-of-core" width="360">
      </a><br>
      <sub>Out-of-core</sub>
    </td>
  </tr>
</table>

## Prerequisites
- CMake 3.20 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)
- OpenGL 3.3 or higher
- GLFW 3.4
- GLM (OpenGL Mathematics library)

Install the following packages:
```bash
sudo apt install cmake build-essential libgl1-mesa-dev libglfw3-dev libglm-dev
```

## Datasets
This rasterizer is compatible with 3D point cloud datasets in `.ply` format. Datasets should be stored in the `data` directory. This implementation is tested with the well-known public 3D point cloud datasets (ground truth `.ply` files) from [Tanks and Temples](https://www.tanksandtemples.org/).

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

## Run

### Default Run
Run the program in the `build` directory:
```bash
./main
```
This will load the default model (e.g., `../data/Church.ply`) with default shaders.

### Command Line Arguments
You can specify custom files via command line arguments:

```bash
./main [options] [model.ply] [shader.vert] [shader.frag]
```

**Available Options:**
- `--test`: Run in test mode (orbital camera pose)
- `--ooc`: Enable out-of-core rendering mode
- `--cache`: (Must be combined with `--ooc`) Enable out-of-core rendering mode with subslots cache.
- `--export`: Captures every rendered frame as a `.png` image in the `outputs/` directory.
- `*.ply`: Specify a PLY model file
- `*.vert`: Specify a custom vertex shader
- `*.frag`: Specify a custom fragment shader

**Examples:**
```bash
# In-core rendering
./main --test

# Out-of-core rendering for large datasets in test mode
./main --test --ooc

# Out-of-core rendering for large datasets in test mode with subslots cache
./main --test --ooc --cache
```

### Camera Controls
- **Normal Mode**: Navigate using standard controls (Camera movements with Q, W, A, S, Z, X. Yaw and pitch control with J, L, I, K)
- **Test Mode**: Enable orbital camera poses (use argument `--test` for this mode)

### Exporting GIFs
When `.png` files are generated, run the following, e.g.:
``` bash
ffmpeg -framerate 60 -i frame_%05d.png \
  -vf "fps=8,scale=360:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
  ../out_OOC.gif
```
to export a `.gif` file for a demo. 

## License
This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgments
- OpenGL for the graphics API
- LearnOpenGL for the awesome guidance
- GLFW for window and input handling
- GLM for various mathematics operations
