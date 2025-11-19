// shell_hook.cpp
#include "shell_hook.h"
#include "download_manager.h"
#include "logging.h"
#include <iostream>
#include <string>
#include <regex>
#include <curl/curl.h>
#include "download_queue.h"

// Removed pragma comment for libcurl.lib to allow project settings to control linking

// Original function pointer
typedef BOOL(WINAPI* pShellExecuteExW)(SHELLEXECUTEINFOW*);
pShellExecuteExW OriginalShellExecuteExW = nullptr;

// Hook storage
BYTE originalBytes[12];
BYTE hookBytes[12];
bool isHooked = false;

// Structure to hold API response data
struct APIResponseData {
    std::string data;
};

// Callback function to write API response data
size_t WriteAPICallback(void* contents, size_t size, size_t nmemb, APIResponseData* response) {
    size_t totalSize = size * nmemb;
    response->data.append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Function to get beatmapset ID from beatmap ID
std::wstring GetBeatmapsetId(const std::wstring& beatmapId) {
    LogDebug("[API] Converting beatmap ID " + std::string(beatmapId.begin(), beatmapId.end()) + " to beatmapset ID...");

    // Convert wstring to string for libcurl
    std::string apiUrl = "https://catboy.best/api/v2/b/" + std::string(beatmapId.begin(), beatmapId.end());
    LogDebug("API URL: " + apiUrl);

    // Initialize CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        LogError("Failed to initialize CURL handle for API request");
        return L"";
    }
    
    // Setup response data structure
    APIResponseData response;
    
    // Configure CURL options
    curl_easy_setopt(curl, CURLOPT_URL, apiUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteAPICallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu! Beatmap Downloader/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); // 30 seconds timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // 10 seconds connection timeout
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    
    // Perform the API request
    CURLcode res = curl_easy_perform(curl);
    
    std::wstring beatmapsetId;
    
    if (res == CURLE_OK) {
        // Get response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (response_code == 200) {
            LogDebug("API Response: " + response.data);

            // Simple string parsing to avoid JSON dependency
            // Look for "beatmapset_id":12345
            std::string searchKey = "\"beatmapset_id\":";
            size_t pos = response.data.find(searchKey);
            if (pos != std::string::npos) {
                size_t start = pos + searchKey.length();
                size_t end = response.data.find_first_of(",}", start);
                if (end != std::string::npos) {
                    std::string idStr = response.data.substr(start, end - start);
                    // Trim whitespace if any
                    idStr.erase(0, idStr.find_first_not_of(" \t\n\r"));
                    idStr.erase(idStr.find_last_not_of(" \t\n\r") + 1);
                    
                    try {
                        int setId = std::stoi(idStr);
                        beatmapsetId = std::to_wstring(setId);
                        LogDebug("Successfully converted to beatmapset ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()));
                    } catch (...) {
                        LogError("Failed to parse beatmapset ID from string: " + idStr);
                    }
                }
            } else {
                LogError("beatmapset_id not found in API response");
            }
        } else {
            LogError("API request failed with HTTP code: " + std::to_string(response_code));
        }
    } else {
        LogError("libcurl API request error: " + std::string(curl_easy_strerror(res)));
    }
    
    curl_easy_cleanup(curl);
    return beatmapsetId;
}

// Helper function to convert wide string to narrow string
std::string WideToNarrow(const wchar_t* str) {
    if (!str) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, str, -1, &strTo[0], size_needed, nullptr, nullptr);
    strTo.resize(strTo.size() - 1); // Remove the extra null terminator
    return strTo;
}

// Hook function
BOOL WINAPI HookedShellExecuteExW(SHELLEXECUTEINFOW* pExecInfo) {
    if (pExecInfo && pExecInfo->lpFile) {
        std::wstring url(pExecInfo->lpFile);
        
        // LOG ALL INTERCEPTED LINKS
        LogDebug("[INTERCEPT] ShellExecuteExW called with: " + std::string(url.begin(), url.end()));

        // Also log additional parameters if they exist
        if (pExecInfo->lpParameters) {
            LogDebug("[INTERCEPT] Parameters: " + WideToNarrow(pExecInfo->lpParameters));
        }
        if (pExecInfo->lpDirectory) {
            LogDebug("[INTERCEPT] Directory: " + WideToNarrow(pExecInfo->lpDirectory));
        }
        if (pExecInfo->lpVerb) {
            LogDebug("[INTERCEPT] Verb: " + WideToNarrow(pExecInfo->lpVerb));
        }
        
        // Check for osu! protocol links (osu://)
        if (url.substr(0, 6) == L"osu://") {
            LogInfo("Detected osu:// protocol link: " + std::string(url.begin(), url.end()));

            // Extract ID from osu:// link - could be beatmap or beatmapset
            std::wregex osuProtocolRegex(L"osu://(?:dl|b|s)/(\\d+)");
            std::wsmatch matches;
            
            if (std::regex_search(url, matches, osuProtocolRegex)) {
                std::wstring extractedId = matches[1].str();
                LogInfo("Extracted ID from osu:// link: " + std::string(extractedId.begin(), extractedId.end()));

                std::wstring beatmapsetId;
                
                // Check if it's a beatmapset link (osu://dl/ or osu://s/)
                if (url.find(L"osu://dl/") != std::wstring::npos || url.find(L"osu://s/") != std::wstring::npos) {
                    // It's already a beatmapset ID
                    beatmapsetId = extractedId;
                    LogInfo("Using beatmapset ID directly: " + std::string(beatmapsetId.begin(), beatmapsetId.end()));
                } else {
                    // It's a beatmap ID (osu://b/), convert to beatmapset ID
                    beatmapsetId = GetBeatmapsetId(extractedId);
                    if (beatmapsetId.empty()) {
                        LogError("Failed to convert beatmap ID to beatmapset ID, falling back to osu! client");
                        // Don't return here, let it fall through to original function
                    }
                }
                
                if (!beatmapsetId.empty()) {
                    // Queue the beatmap for download
                    DownloadQueue::Instance().Push(beatmapsetId);
                    LogInfo("Beatmap download queued from osu:// link");
                    return TRUE; // Prevent osu! client from handling
                }
            }
        }
        
        // Check for HTTP(S) osu! beatmap links
        // Updated regex to handle both beatmap and beatmapset links
        std::wregex beatmapsetRegex(L"(?:https?://)?osu\\.ppy\\.sh/(?:beatmapsets|s)/(\\d+)");
        std::wregex beatmapRegex(L"(?:https?://)?osu\\.ppy\\.sh/(?:beatmaps|b)/(\\d+)");
        std::wsmatch matches;
        
        std::wstring beatmapsetId;
        
        // Check for beatmapset links first
        if (std::regex_search(url, matches, beatmapsetRegex)) {
            beatmapsetId = matches[1].str();
            LogInfo("[INFO] Intercepted HTTP(S) beatmapset link, ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()));
        }
        // Check for individual beatmap links
        else if (std::regex_search(url, matches, beatmapRegex)) {
            std::wstring beatmapId = matches[1].str();
            LogInfo("[INFO] Intercepted HTTP(S) beatmap link, ID: " + std::string(beatmapId.begin(), beatmapId.end()));

            // Convert beatmap ID to beatmapset ID
            beatmapsetId = GetBeatmapsetId(beatmapId);
            if (beatmapsetId.empty()) {
                LogError("Failed to convert beatmap ID to beatmapset ID, falling back to browser");
                // Don't return here, let it fall through to original function
            }
        }
        
        if (!beatmapsetId.empty()) {
            // Queue the beatmap for download
            DownloadQueue::Instance().Push(beatmapsetId);
            LogInfo("Beatmap download queued from HTTP(S) link");
            return TRUE; // Prevent browser from opening
        }
    }
    
    // Temporarily unhook to call original function
    if (isHooked) {
        DWORD oldProtect;
        VirtualProtect(OriginalShellExecuteExW, sizeof(originalBytes), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(OriginalShellExecuteExW, originalBytes, sizeof(originalBytes));
        VirtualProtect(OriginalShellExecuteExW, sizeof(originalBytes), oldProtect, &oldProtect);
    }
    
    // Call original function
    BOOL result = OriginalShellExecuteExW(pExecInfo);
    
    // Re-hook
    if (isHooked) {
        DWORD oldProtect;
        VirtualProtect(OriginalShellExecuteExW, sizeof(hookBytes), PAGE_EXECUTE_READWRITE, &oldProtect);
        memcpy(OriginalShellExecuteExW, hookBytes, sizeof(hookBytes));
        VirtualProtect(OriginalShellExecuteExW, sizeof(hookBytes), oldProtect, &oldProtect);
    }
    
    return result;
}

bool HookShellExecute() {
    // Get the address of ShellExecuteExW
    HMODULE hShell32 = GetModuleHandle(L"shell32.dll");
    if (!hShell32) {
        LogError("Failed to get shell32.dll handle");
        return false;
    }
    
    OriginalShellExecuteExW = (pShellExecuteExW)GetProcAddress(hShell32, "ShellExecuteExW");
    if (!OriginalShellExecuteExW) {
        LogError("Failed to get ShellExecuteExW address");
        return false;
    }

    LogDebug("ShellExecuteExW address: " + std::to_string((uintptr_t)OriginalShellExecuteExW));
    
    // Change memory protection
    DWORD oldProtect;
    if (!VirtualProtect(OriginalShellExecuteExW, sizeof(originalBytes), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        LogError("Failed to change memory protection. Error: " + std::to_string(GetLastError()));
        return false;
    }
    
    // Save original bytes
    memcpy(originalBytes, OriginalShellExecuteExW, sizeof(originalBytes));
    
    // Create hook bytes (x86 relative jump)
    DWORD_PTR hookAddress = (DWORD_PTR)HookedShellExecuteExW;
    DWORD_PTR originalAddress = (DWORD_PTR)OriginalShellExecuteExW;
    DWORD relativeAddress = hookAddress - originalAddress - 5;
    
    // Prepare hook bytes: JMP relative32
    hookBytes[0] = 0xE9; // JMP instruction
    memcpy(&hookBytes[1], &relativeAddress, 4);
    // Fill remaining bytes with NOPs
    for (int i = 5; i < sizeof(hookBytes); i++) {
        hookBytes[i] = 0x90; // NOP
    }
    
    // Install the hook
    memcpy(OriginalShellExecuteExW, hookBytes, sizeof(hookBytes));
    
    // Restore memory protection
    VirtualProtect(OriginalShellExecuteExW, sizeof(hookBytes), oldProtect, &oldProtect);
    
    isHooked = true;
    LogInfo("ShellExecuteExW hooked successfully using inline hooking");
    return true;
}

void UnhookShellExecute() {
    if (!isHooked || !OriginalShellExecuteExW) {
        return;
    }
    
    // Change memory protection
    DWORD oldProtect;
    if (VirtualProtect(OriginalShellExecuteExW, sizeof(originalBytes), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // Restore original bytes
        memcpy(OriginalShellExecuteExW, originalBytes, sizeof(originalBytes));
        
        // Restore memory protection
        VirtualProtect(OriginalShellExecuteExW, sizeof(originalBytes), oldProtect, &oldProtect);
        
        isHooked = false;
        LogInfo("ShellExecuteExW unhooked successfully");
    } else {
        LogError("Failed to unhook ShellExecuteExW");
    }
}