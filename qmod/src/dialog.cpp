#include "dialog.hpp"
#include <thread>

// TODO: Is there a way to inject the host URI at build-time?
const char* TOMBSTONE_POST_URL = "http://mods.quest/upload-tombstone";
// const char* TOMBSTONE_POST_URL = "http://192.168.1.16:3000/upload-tombstone";

static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void uploadCrashLog(
    const char* tombstonePath,
    // TODO: these types don't line up and it feels bad
    std::function<void(std::string responseText)> onSuccess,
    std::function<void(const char* errorMessage)> onFailure
) {
    std::string responseText;

    auto curl = curl_easy_init();
    auto form = curl_mime_init(curl);

    auto field = curl_mime_addpart(form);
    curl_mime_name(field, "tombstone");
    curl_mime_filedata(field, tombstonePath);

    static auto gameVersion = UnityEngine::Application::get_version();
    field = curl_mime_addpart(form);
    curl_mime_name(field, "version");
    curl_mime_data(field, to_utf8(csstrtostr(gameVersion)).c_str(), CURL_ZERO_TERMINATED);

    static auto deviceUniqueIdentifier = reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetDeviceUniqueIdentifier()"))();
    field = curl_mime_addpart(form);
    curl_mime_name(field, "uid");
    curl_mime_data(field, to_utf8(csstrtostr(deviceUniqueIdentifier)).c_str(), CURL_ZERO_TERMINATED);

    static auto operatingSystem = reinterpret_cast<function_ptr_t<Il2CppString*>>(il2cpp_functions::resolve_icall("UnityEngine.SystemInfo::GetOperatingSystem()"))();
    field = curl_mime_addpart(form);
    curl_mime_name(field, "os");
    curl_mime_data(field, to_utf8(csstrtostr(operatingSystem)).c_str(), CURL_ZERO_TERMINATED);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_URL, TOMBSTONE_POST_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseText);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_mime_free(form);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        onFailure(curl_easy_strerror(res));
    } else {
        onSuccess(responseText);
    }
}

void showCrashReportDialog(const char* tombstonePath) {
    // TODO: Generally improve visuals here
    auto container = QuestUI::BeatSaberUI::CreateFloatingScreen(
        UnityEngine::Vector2(3.0f, 6.0f),
        UnityEngine::Vector3(0.0f, 1.5f, 2.5f),
        UnityEngine::Vector3(0.0f, 0.0f, 0.0f),
        0.0f,
        true
    );
    // TODO: Background isn't working :(
    // container
    //     ->AddComponent<QuestUI::Backgroundable*>()
    //     ->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.96f);

    auto text = QuestUI::BeatSaberUI::CreateText(
        container->get_transform(),
        "It looks like your game crashed recently!\nWould you like to upload an error report\nto help diagnose and fix this issue?",
        UnityEngine::Vector2(0.0f, 0.0f)
    );
    // text->set_lineSpacing(0.1f);

    UnityEngine::UI::Button* cancelButton;
    UnityEngine::UI::Button* confirmButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Send Error Report",
        UnityEngine::Vector2(-16.5f, -18.0f),
        UnityEngine::Vector2(32.0f, 10.0f),
        [=]() {
            // TODO: Putting this in a thread crashes here
            // std::thread(
            uploadCrashLog(
                tombstonePath,
                [=](std::string responseText) {
                    getLogger().info("Tombstone uploaded successfully");
                    text->set_text(il2cpp_utils::createcsstr("Success! Your log can be found at:\nhttp://mods.quest/tombstones/" + responseText));
                    // TODO: Changing cancelButton text or removing confirmButton both crash here
                },
                [=](const char* errorMessage) {
                    getLogger().info("Web request failed with error: %s", errorMessage);
                    text->set_text(il2cpp_utils::createcsstr("Error uploading log!\n" + std::string(errorMessage)));
                }
            );           
        }
    );

    cancelButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Don't Send",
        UnityEngine::Vector2(16.5f, -18.0f),
        UnityEngine::Vector2(32.0f, 10.0f),
        [=]() {
            UnityEngine::Object::Destroy(container);
        }
    );
}
