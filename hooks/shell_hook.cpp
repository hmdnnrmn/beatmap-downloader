// shell_hook.cpp
#include "shell_hook.h"
#include "features/download_manager.h"
#include "utils/logging.h"
#include "network/HttpRequest.h"
#include <iostream>
#include <string>
#include <regex>
#include "features/download_queue.h"

// Original function pointer
typedef BOOL(WINAPI* pShellExecuteExW)(SHELLEXECUTEINFOW*);
pShellExecuteExW OriginalShellExecuteExW = nullptr;

// Hook storage
BYTE originalBytes[12];
BYTE hookBytes[12];
bool isHooked = false;

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

                bool isBeatmapId = false;
                
                // Check if it's a beatmapset link (osu://dl/ or osu://s/)
                if (url.find(L"osu://dl/") != std::wstring::npos || url.find(L"osu://s/") != std::wstring::npos) {
                    // It's already a beatmapset ID
                    isBeatmapId = false;
                    LogInfo("Detected beatmapset ID: " + std::string(extractedId.begin(), extractedId.end()));
                } else {
                    // It's a beatmap ID (osu://b/)
                    isBeatmapId = true;
                    LogInfo("Detected beatmap ID: " + std::string(extractedId.begin(), extractedId.end()));
                }
                
                // Queue the beatmap for download
                DownloadQueue::Instance().Push(extractedId, isBeatmapId);
                LogInfo("Beatmap download queued from osu:// link");
                return TRUE; // Prevent osu! client from handling
            }
        }
        
        // Check for HTTP(S) osu! beatmap links
        // Updated regex to handle both beatmap and beatmapset links
        std::wregex beatmapsetRegex(L"(?:https?://)?osu\\.ppy\\.sh/(?:beatmapsets|s)/(\\d+)");
        std::wregex beatmapRegex(L"(?:https?://)?osu\\.ppy\\.sh/(?:beatmaps|b)/(\\d+)");
        std::wsmatch matches;
        
        // Check for beatmapset links first
        if (std::regex_search(url, matches, beatmapsetRegex)) {
            std::wstring id = matches[1].str();
            LogInfo("[INFO] Intercepted HTTP(S) beatmapset link, ID: " + std::string(id.begin(), id.end()));
            DownloadQueue::Instance().Push(id, false);
            return TRUE;
        }
        // Check for individual beatmap links
        else if (std::regex_search(url, matches, beatmapRegex)) {
            std::wstring id = matches[1].str();
            LogInfo("[INFO] Intercepted HTTP(S) beatmap link, ID: " + std::string(id.begin(), id.end()));
            DownloadQueue::Instance().Push(id, true);
            return TRUE;
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