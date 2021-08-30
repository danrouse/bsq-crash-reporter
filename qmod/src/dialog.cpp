#include "dialog.hpp"

// TODO: Is there a way to inject the host URI at build-time?
const char* TOMBSTONE_POST_URL = "http://192.168.1.16:3000/upload";

void uploadCrashLog(const char* tombstonePath) {
    getLogger().debug("upload crash log %s", tombstonePath);

    auto *curl = curl_easy_init();
    auto form = curl_mime_init(curl);

    auto field = curl_mime_addpart(form);
    curl_mime_name(field, "tombstone");
    curl_mime_filedata(field, tombstonePath);

    // TODO: Include real metadata with the request
    // - List of running mods (not exactly same as at crash time, but better than nothing)
    // - Hardware identifier (UnityEngine.SystemInfo.deviceUniqueIdentifier seems to be stripped?)

    // field = curl_mime_addpart(form);
    // curl_mime_name(field, "some-metadata");
    // curl_mime_data(field, "some value", CURL_ZERO_TERMINATED);

    struct curl_slist *headerlist = NULL;
    static const char buf[] = "Expect:";
    headerlist = curl_slist_append(headerlist, buf);
    curl_easy_setopt(curl, CURLOPT_URL, TOMBSTONE_POST_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        getLogger().debug("cURL error: %s", curl_easy_strerror(res));
    } else {
        getLogger().debug("curl request OK");
    }

    curl_easy_cleanup(curl);
    curl_mime_free(form);
    curl_slist_free_all(headerlist);
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
            uploadCrashLog(tombstonePath);
            // TODO: Actually destroy the container when not in development
            // UnityEngine::Object::Destroy(container);
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
