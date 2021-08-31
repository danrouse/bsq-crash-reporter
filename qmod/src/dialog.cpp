#include "dialog.hpp"

// TODO: Is there a way to inject the host URI at build-time?
const char* TOMBSTONE_POST_URL = "http://34.74.215.247/upload";

void uploadCrashLog(
    const char* tombstonePath,
    std::function<void()> onSuccess,
    std::function<void(const char* errorMessage)> onFailure
) {
    auto curl = curl_easy_init();
    auto form = curl_mime_init(curl);

    auto field = curl_mime_addpart(form);
    curl_mime_name(field, "tombstone");
    curl_mime_filedata(field, tombstonePath);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_URL, TOMBSTONE_POST_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_mime_free(form);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        onFailure(curl_easy_strerror(res));
    } else {
        onSuccess();
    }
}

void showCrashReportDialog(const char* tombstonePath) {
    // TODO: Generally improve visuals here
    auto container = QuestUI::BeatSaberUI::CreateFloatingScreen(
        UnityEngine::Vector2(6.0f, 6.0f),
        UnityEngine::Vector3(0.0f, 1.5f, 3.0f),
        UnityEngine::Vector3(0.0f, 0.0f, 0.0f)
    );
    // TODO: Background isn't working :(
    // container
    //     ->AddComponent<QuestUI::Backgroundable*>()
    //     ->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.96f);

    auto text = QuestUI::BeatSaberUI::CreateText(
        container->get_transform(),
        "It looks like your game crashed recently!\nWould you like to upload an error report to help diagnose and fix this issue?",
        UnityEngine::Vector2(0.0f, 0.0f)
    );
    text->set_lineSpacing(0.1f);

    auto confirmButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Send Error Report",
        UnityEngine::Vector2(-20.0f, -10.0f),
        [tombstonePath, container]() {
            uploadCrashLog(tombstonePath, [container]() {
                getLogger().info("Tombstone uploaded successfully");
                // UnityEngine::Object::Destroy(container);
            }, [container](const char* errorMessage) {
                getLogger().info("Web request failed with error: %s", errorMessage);
                // UnityEngine::Object::Destroy(container);
            });            
        }
    );

    auto cancelButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Don't Send",
        UnityEngine::Vector2(20.0f, -10.0f),
        [container]() {
            UnityEngine::Object::Destroy(container);
        }
    );
}
