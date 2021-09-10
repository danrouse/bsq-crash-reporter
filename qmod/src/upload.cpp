#include "upload.hpp"

// TODO: Is there a way to inject the host URI at build-time?
const char* URL_POST_CRASH_LOG = "https://mods.quest/upload-crash-details";
// const char* URL_POST_CRASH_LOG = "http://192.168.1.16:3000/upload-crash-details";

static size_t cURLWriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size* nmemb);
    return size * nmemb;
}

cURLResult uploadCrashLog(FormFields fields) {
    std::string responseText;

    auto curl = curl_easy_init();
    auto form = curl_mime_init(curl);

    for (const auto& field : fields) {
      auto curlField = curl_mime_addpart(form);
      curl_mime_name(curlField, field.first);
      curl_mime_data(curlField, field.second.c_str(), CURL_ZERO_TERMINATED);
    }

    // TODO? Oh no this is so bad...
    // Trying to save a CA cert at runtime was failing, though,
    // and Android system CA certs didn't appear to work.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_URL, URL_POST_CRASH_LOG);
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
