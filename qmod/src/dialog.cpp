#include "dialog.hpp"

// TODO: Is there a way to inject the host URI at build-time?
const char* TOMBSTONE_POST_URL = "https://mods.quest/upload-tombstone";
// const char* TOMBSTONE_POST_URL = "http://192.168.1.16:3000/upload-tombstone";

static size_t cURLWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size* nmemb);
    return size * nmemb;
}

struct cURLResult {
    bool success;
    std::string text;
};

cURLResult uploadCrashLog(const char* tombstonePath) {
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

    // TODO? Oh no this is so bad...
    // Trying to save a CA cert at runtime was failing, though,
    // and Android system CA certs didn't appear to work.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_URL, TOMBSTONE_POST_URL);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cURLWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseText);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_mime_free(form);
    curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        return {false, std::string(curl_easy_strerror(res))};
    } else {
        return {true, responseText};
    }
}

// Q: if these buttons are defined inside showCrashReportDialog,
//    then the game crashes when trying to call methods on them
//    in the callback lambda
UnityEngine::UI::Button* confirmButton;
UnityEngine::UI::Button* cancelButton;
bool isUploadInProgress;

void showCrashReportDialog(const char* tombstonePath) {
    auto container = QuestUI::BeatSaberUI::CreateCanvas();
    container->get_transform()->set_position(UnityEngine::Vector3(0.0f, 1.5f, 2.5f));
    container->GetComponent<UnityEngine::RectTransform*>()->set_sizeDelta(UnityEngine::Vector2(74.0f, 30.0f));
    auto background = container->AddComponent<QuestUI::Backgroundable*>();
    static auto backgroundName = il2cpp_utils::createcsstr("round-rect-panel", il2cpp_utils::StringType::Manual);
    background->ApplyBackgroundWithAlpha(il2cpp_utils::createcsstr("round-rect-panel"), 0.95f);

    auto dialogText = QuestUI::BeatSaberUI::CreateText(
        container->get_transform(),
        "<line-height=80%>It looks like your game crashed recently!\n"
        "Would you like to upload an error report\nto help diagnose and fix this issue?",
        UnityEngine::Vector2(-2.0f, 7.0f)
    );
    
    confirmButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Send Error Report",
        "PlayButton",
        UnityEngine::Vector2(-15.5f, -8.0f),
        UnityEngine::Vector2(34.0f, 10.0f),
        [=]() {
            // only allow a single upload
            if (isUploadInProgress) return;
            isUploadInProgress = true;

            // start web request on another thread
            auto fut = std::async<function_ptr_t<cURLResult, const char*>>(
                std::launch::async,
                &uploadCrashLog,
                tombstonePath
            );
            QuestUI::BeatSaberUI::SetButtonText(confirmButton, "Uploading...");

            // put coroutine on a random MB to wait for web request to finish
            background->StartCoroutine(reinterpret_cast<System::Collections::IEnumerator*>(custom_types::Helpers::CoroutineHelper::New(
                waitForFuture<cURLResult>(std::move(fut), [=](cURLResult res) {
                    isUploadInProgress = false;
                    if (res.success) {
                        getLogger().info("Tombstone uploaded successfully");
                        dialogText->set_text(il2cpp_utils::createcsstr(
                            "<line-height=120%>Success! Your log can be found at:\n"
                            "<size=80%><align=\"center\">https://mods.quest/tombstones/" + res.text));
                        confirmButton->get_gameObject()->set_active(false);
                        auto cancelButtonTransform = cancelButton->GetComponent<UnityEngine::RectTransform*>();
                        cancelButtonTransform->set_sizeDelta(UnityEngine::Vector2(64.0f, 10.0f));
                        cancelButtonTransform->set_anchoredPosition(UnityEngine::Vector2(0.0f, -8.0f));
                        QuestUI::BeatSaberUI::SetButtonText(cancelButton, "Close");
                    } else {
                        getLogger().info("Web request failed with error: %s", res.text.c_str());
                        dialogText->set_text(il2cpp_utils::createcsstr("Error uploading log!\n" + res.text));
                    }
                })
            )));
        }
    );

    cancelButton = QuestUI::BeatSaberUI::CreateUIButton(
        container->get_transform(),
        "Don't Send",
        UnityEngine::Vector2(17.5f, -8.0f),
        UnityEngine::Vector2(30.0f, 10.0f),
        [container]() { UnityEngine::Object::Destroy(container); }
    );
}
