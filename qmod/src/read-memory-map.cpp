#include "read-memory-map.hpp"

std::unordered_set<std::string> getLoadedMods() {
  std::unordered_set<std::string> libraries;
  std::ifstream reader("/proc/self/maps");
  std::string line;
  while (std::getline(reader, line)) {
    if (line.ends_with(".so") && line.find("com.beatgames.beatsaber/") != std::string::npos) {
      libraries.insert(line.substr(line.find_first_of("/")));
    }
  }
  reader.close();
  return libraries;
}
