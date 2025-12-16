#include "Rasterizer.h"
#include <string>

bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char **argv) {

  // default files
  std::string read_shader_vert = "../src/shader/shader.vert";
  std::string read_shader_frag = "../src/shader/shader.frag";
  std::string read_filename = "../data/Church.ply";
  bool testmode = false;

  for (int i = 1; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "--test"){
      testmode = true;
      continue;
    }
    if (ends_with(arg, ".ply")){
      read_filename = argv[i];
      continue;
    }
    if (ends_with(arg, ".vert")){
      read_shader_vert = argv[i];
      continue;
    }
    if (ends_with(arg, ".frag")){
      read_shader_frag = argv[i];
      continue;
    }
  }
  Rasterizer rasterizer(testmode, 800, 600, 1.0, 100.0);
  if (!rasterizer.init(read_filename, read_shader_vert, read_shader_frag)) {
    std::cout << "Failed to initialize Rasterizer." << std::endl;
    return 1;
  }
  rasterizer.render();

  return 0;
}
