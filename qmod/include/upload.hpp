#pragma once

#include "libcurl/shared/curl.h"
#include <string>
#include <vector>

typedef std::vector<std::pair<const char*, std::string>> FormFields;
struct cURLResult {
    bool success;
    std::string text;
};

cURLResult uploadCrashLog(FormFields fields);
