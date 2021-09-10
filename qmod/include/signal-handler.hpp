#pragma once

#include <csignal>
#include <signal.h>
#include <time.h>

#include "beatsaber-hook/shared/utils/hooking.hpp"

#include "logger.hpp"
#include "backtrace.hpp"
#include "read-logcat.hpp"
#include "read-memory-map.hpp"
#include "elf-build-id.hpp"
#include "upload.hpp"

struct GameState {
  std::string gameVersion;
  std::string deviceUniqueIdentifier;
  std::string operatingSystem;
  std::string prevSceneName;
  std::string nextSceneName;
  int currentSceneTime = 0;
};
