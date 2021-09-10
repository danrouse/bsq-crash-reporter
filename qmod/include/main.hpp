#pragma once

#include <csignal>
#include <signal.h>
#include <unordered_map>
#include <time.h>

#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "beatsaber-hook/shared/utils/utils-functions.h"

#include "logger.hpp"
#include "backtrace.hpp"
#include "read-logcat.hpp"
#include "read-memory-map.hpp"
#include "elf-build-id.hpp"
#include "upload.hpp"
