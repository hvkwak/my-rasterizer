#include "Rasterizer.h"
#include <string>

bool ends_with(const std::string& s, const std::string& suffix) {
  return s.size() >= suffix.size() && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}


int main(int argc, char **argv) {

  // default settings
  std::string shader_vert = "../src/shader/shader.vert";
  std::string shader_frag = "../src/shader/shader.frag";
  std::string plyPath = "../data/Church.ply";
  std::string outDir = "";
  bool test = false;
  bool ooc = false;

  for (int i = 1; i < argc; i++){
    std::string arg = argv[i];
    if (arg == "--test"){
      test = true;
      continue;
    }
    if (arg == "--ooc"){
      // out-of-core mode: points will be seperated into blocks in the directory "data"
      outDir = "../data";
      ooc = true;
      continue;
    }
    if (ends_with(arg, ".ply")){
      plyPath = argv[i];
      continue;
    }
    if (ends_with(arg, ".vert")){
      shader_vert = argv[i];
      continue;
    }
    if (ends_with(arg, ".frag")){
      shader_frag = argv[i];
      continue;
    }
  }
  Rasterizer rasterizer;
  if (!rasterizer.init(plyPath, outDir, shader_vert, shader_frag, test, ooc, 800, 600, 1.0, 100.0, 10.0)) {
    std::cout << "Failed to initialize Rasterizer." << std::endl;
    return 1;
  }
  rasterizer.render();

  return 0;
}
