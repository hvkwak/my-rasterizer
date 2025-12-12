#include "Rasterizer.h"
#include <string>

int main(int argc, char **argv) {

  // get the file names
  std::string read_filename;
  std::string read_shader_vert;
  std::string read_shader_frag;
  if (argc == 1){
    // default
    read_filename = "../data/Barn.ply";
    read_shader_vert = "../src/shader/shader.vert";
    read_shader_frag = "../src/shader/shader.frag";
  }
  if (argc == 2){
    read_filename = argv[1];
    read_shader_vert = argv[2];
    read_shader_frag = argv[3];
  }

  Rasterizer rasterizer;
  if (!rasterizer.init(read_filename, read_shader_vert, read_shader_frag)) {
    std::cout << "Failed to initialize Rasterizer." << std::endl;
    return 1;
  }
  rasterizer.render();

  return 0;
}
