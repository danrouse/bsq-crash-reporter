#pragma once

#include <elf.h>
#include <string>
#include <stdio.h>
#include <unordered_set>
#include <unordered_map>

#include "beatsaber-hook/shared/utils/utils-functions.h"

std::pair<std::string, std::string> getLibraryDataStrings(std::unordered_set<std::string> libraries);

std::string getBuildId(const char* elfPath);
