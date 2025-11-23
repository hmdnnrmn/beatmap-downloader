#include "HttpRequest.h"
#include <curl/curl.h>
#include <fstream>
#include <thread>
#include <iostream>
#include <windows.h> // For DeleteFileW

namespace network {

    // Helper for writing to string
    static size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Helper for writing to file
    static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        std::ofstream* file = (std::ofstream*)userp;
        file->write((char*)contents, size * nmemb);
        return size * nmemb;
    }

    // Helper for progress
    struct ProgressData {
        HttpRequest::ProgressCallback callback;
    };

    static int ProgressCallbackWrapper(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        ProgressData* data = (ProgressData*)clientp;
        if (data->callback && dltotal > 0) {
            data->callback((double)dlnow, (double)dltotal);
        }
        return 0;
    }

    void HttpRequest::GlobalInit() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    void HttpRequest::GlobalCleanup() {
        curl_global_cleanup();
    }

    bool HttpRequest::Download(const std::string& url, const std::wstring& destPath, 
                               ProgressCallback progressCb, std::string* outError) {
        CURL* curl = curl_easy_init();
        if (!curl) {
            if (outError) *outError = "Failed to init curl";
            return false;
        }

        std::ofstream outFile(destPath, std::ios::binary);
        if (!outFile.is_open()) {
            if (outError) *outError = "Failed to open file";
            curl_easy_cleanup(curl);
            return false;
        }

        ProgressData progData = { progressCb };

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outFile);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, ProgressCallbackWrapper);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progData);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // For compatibility
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu! Beatmap Downloader/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        
        outFile.close();
        
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        bool success = (res == CURLE_OK) && (response_code >= 200 && response_code < 300);

        if (!success) {
            if (outError) {
                if (res != CURLE_OK) *outError = curl_easy_strerror(res);
                else *outError = "HTTP " + std::to_string(response_code);
            }
            // Delete partial file
            DeleteFileW(destPath.c_str());
        }

        curl_easy_cleanup(curl);
        return success;
    }

    bool HttpRequest::Get(const std::string& url, std::string& outResponse, std::string* outError) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outResponse);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu! Beatmap Downloader/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        bool success = (res == CURLE_OK) && (response_code >= 200 && response_code < 300);

        if (!success && outError) {
             if (res != CURLE_OK) *outError = curl_easy_strerror(res);
             else *outError = "HTTP " + std::to_string(response_code);
        }

        curl_easy_cleanup(curl);
        return success;
    }

    bool HttpRequest::GetRedirectUrl(const std::string& url, std::string& outUrl, std::string* outError) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L); // Do not follow redirect
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);         // HEAD request
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu! Beatmap Downloader/1.0");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        
        if (res != CURLE_OK) {
            if (outError) *outError = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            return false;
        }

        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code >= 300 && response_code < 400) {
            char* location = NULL;
            curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL, &location);
            if (location) {
                outUrl = location;
                curl_easy_cleanup(curl);
                return true;
            }
        }

        if (outError) *outError = "No redirect found (HTTP " + std::to_string(response_code) + ")";
        curl_easy_cleanup(curl);
        return false;
    }

    void HttpRequest::DownloadAsync(const std::string& url, const std::wstring& destPath,
                                    ProgressCallback progressCb, CompletionCallback completionCb) {
        std::thread([=]() {
            std::string error;
            bool success = Download(url, destPath, progressCb, &error);
            if (completionCb) completionCb(success, error);
        }).detach();
    }

    void HttpRequest::GetAsync(const std::string& url, CompletionCallback completionCb) {
        std::thread([=]() {
            std::string response;
            std::string error;
            bool success = Get(url, response, &error);
            if (completionCb) completionCb(success, success ? response : error);
        }).detach();
    }

}
