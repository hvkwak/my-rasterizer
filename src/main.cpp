#include "Rasterizer.h"
#include <string>

int main(int argc, char **argv) {

  // get the file names
  std::string read_shader_vert = "../src/shader/shader.vert";
  std::string read_shader_frag = "../src/shader/shader.frag";
  std::string read_filename;
  if (argc == 1){
    // default.
    read_filename = "../data/Barn.ply";
  }
  if (argc == 2){
    read_filename = argv[1];
  }
  Rasterizer rasterizer;
  if (!rasterizer.init(read_filename, read_shader_vert, read_shader_frag)) {
    std::cout << "Failed to initialize Rasterizer." << std::endl;
    return 1;
  }
  rasterizer.render();

  return 0;
}
