#pragma once

#include <string>
#include <vector>
#include <functional>
#include <filesystem>
#include <future>

namespace network {

    class HttpRequest {
    public:
        // Callback types
        using ProgressCallback = std::function<void(double dlNow, double dlTotal)>;
        using CompletionCallback = std::function<void(bool success, const std::string& errorOrData)>;

        static void GlobalInit();
        static void GlobalCleanup();

        // Synchronous methods
        static bool Download(const std::string& url, const std::wstring& destPath, 
                             ProgressCallback progressCb = nullptr, 
                             std::string* outError = nullptr,
                             long timeoutSeconds = 300);

        static bool Get(const std::string& url, std::string& outResponse, 
                        std::string* outError = nullptr);

        static bool GetRedirectUrl(const std::string& url, std::string& outUrl, 
                                   std::string* outError = nullptr);

        // Asynchronous methods
        static void DownloadAsync(const std::string& url, const std::wstring& destPath,
                                  ProgressCallback progressCb,
                                  CompletionCallback completionCb,
                                  long timeoutSeconds = 300);

        static void GetAsync(const std::string& url, 
                             CompletionCallback completionCb);
    };

}
