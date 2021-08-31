#pragma once

#include "questui/shared/BeatSaberUI.hpp"
#include "libcurl/shared/curl.h"
#include <sys/stat.h>

#include "logger.hpp"

void showCrashReportDialog(const char* tombstonePath);
void uploadCrashLog(
    const char* tombstonePath,
    std::function<void()> onSuccess,
    std::function<void(const char* errorMessage)> onFailure
);
