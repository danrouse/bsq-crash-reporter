#pragma once

#include <thread>
#include <future>

#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/Backgroundable.hpp"
#include "custom-types/shared/coroutine.hpp"
#include "libcurl/shared/curl.h"

#include "UnityEngine/Application.hpp"
#include "UnityEngine/WaitForEndOfFrame.hpp"
#include "System/Collections/IEnumerator.hpp"

#include "logger.hpp"

void showCrashReportDialog(const char* tombstonePath);
void uploadCrashLog(
    const char* tombstonePath,
    std::function<void(std::string responseText)> onSuccess,
    std::function<void(const char* errorMessage)> onFailure
);

template<typename T>
custom_types::Helpers::Coroutine waitForFuture(std::future<T> fut, std::function<void(T)> onComplete) {
    while (fut.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
        co_yield reinterpret_cast<System::Collections::IEnumerator*>(CRASH_UNLESS(UnityEngine::WaitForEndOfFrame::New_ctor()));
    }
    onComplete(fut.get());
    co_return;
}
