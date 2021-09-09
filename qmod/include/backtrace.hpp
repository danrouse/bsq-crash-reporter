#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <unwind.h>
#include <cxxabi.h>
#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <dlfcn.h>
#include <unordered_set>

#include "beatsaber-hook/shared/utils/utils-functions.h"
#include "elf-build-id.hpp"

struct BacktraceState {
    bool first;
    uintptr_t firstPC;
    void **current;
    void **end;
};

struct BacktraceDetails {
    std::string lines;
    std::unordered_set<std::string> libraries;
};

BacktraceDetails getBacktraceLines(uint16_t frameCount, uintptr_t targetPc, int framesToSkip = 0);
