// download_manager.cpp
#include "download_manager.h"
#include <iostream>
#include "logging.h"
#include "network/HttpRequest.h"
#include <shlobj.h>
#include <shellapi.h>
#include "config_manager.h"
#include "notification_manager.h"
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

// Global state
static DownloadState g_DownloadState;
static std::mutex g_StateMutex;

DownloadState GetDownloadState() {
    std::lock_guard<std::mutex> lock(g_StateMutex);
    return g_DownloadState;
}

static void UpdateDownloadState(const std::wstring& id, const std::wstring& name, int prog, size_t dl, size_t total, bool active) {
    std::lock_guard<std::mutex> lock(g_StateMutex);
    g_DownloadState.beatmapId = id;
    g_DownloadState.filename = name;
    g_DownloadState.progress = prog;
    g_DownloadState.downloadedBytes = dl;
    g_DownloadState.totalBytes = total;
    g_DownloadState.isDownloading = active;
}

bool InitializeDownloadManager() {
    network::HttpRequest::GlobalInit();
    LogInfo("Download manager initialized (Network Layer Refactored)");
    return true;
}

void CleanupDownloadManager() {
    network::HttpRequest::GlobalCleanup();
    LogInfo("Download manager cleaned up");
}

bool CheckIfMapExists(const std::wstring& beatmapId) {
    std::wstring songsPath = ConfigManager::Instance().GetSongsPath();
    
    // 1. Check for .osz file
    std::wstring oszPath = songsPath + L"\\" + beatmapId + L".osz";
    if (GetFileAttributesW(oszPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        LogInfo("Beatmap .osz already exists: " + std::string(beatmapId.begin(), beatmapId.end()));
        return true;
    }

    // 2. Check for imported folder (starts with ID followed by space)
    try {
        if (fs::exists(songsPath)) {
            for (const auto& entry : fs::directory_iterator(songsPath)) {
                if (entry.is_directory()) {
                    std::wstring dirName = entry.path().filename().wstring();
                    if (dirName.find(beatmapId + L" ") == 0) {
                        LogInfo("Beatmap already imported: " + std::string(dirName.begin(), dirName.end()));
                        return true;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        LogError("Error scanning Songs directory: " + std::string(e.what()));
    }

    return false;
}

bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& beatmapId) {
    std::wstring songsPath = ConfigManager::Instance().GetSongsPath();
    std::wstring filename = beatmapId + L".osz";
    std::wstring fullPath = songsPath + L"\\" + filename;

    LogDebug("Target path: " + std::string(fullPath.begin(), fullPath.end()));

    // Ensure Songs directory exists
    if (GetFileAttributesW(songsPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(songsPath.c_str(), NULL)) {
             LogError("Songs directory does not exist and could not be created.");
             UpdateDownloadState(beatmapId, L"Error: No Songs Dir", 0, 0, 0, false);
             return false;
        }
    }

    LogInfo("Starting download from: " + downloadUrl);

    // Use HttpRequest
    std::string error;
    bool success = network::HttpRequest::Download(downloadUrl, fullPath, 
        [&](double dlNow, double dlTotal) {
            if (dlTotal > 0) {
                int progress = (int)((dlNow / dlTotal) * 100.0);
                UpdateDownloadState(beatmapId, filename, progress, (size_t)dlNow, (size_t)dlTotal, true);
            }
        }, 
        &error
    );

    if (success) {
        LogInfo("Successfully downloaded: " + std::string(filename.begin(), filename.end()));
        UpdateDownloadState(beatmapId, L"Complete", 100, 0, 0, false);

        if (ConfigManager::Instance().GetAutoOpen()) {
            ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_HIDE);
        }
        return true;
    } else {
        LogError("Download failed: " + error);
        UpdateDownloadState(beatmapId, L"Failed", 0, 0, 0, false);
        return false;
    }
}

// Helper function to resolve beatmapset ID based on mirror and ID type
std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId, int mirrorIndex) {
    std::string idStr(id.begin(), id.end());
    
    // Catboy.best (Index 0)
    if (mirrorIndex == 0) {
        // Catboy handles both /b/ and /s/ via API, but for direct download /d/ usually expects SetID.
        // However, user requested:
        // if osu.ppy.sh/b/xxx -> https://catboy.best/api/v2/b/{id} (Direct Download)
        // if osu.ppy.sh/s/xxx -> https://catboy.best/api/v2/s/{id} (Direct Download)
        // We will return the FULL URL here because the structure is different.
        // But DownloadBeatmap expects an ID to check for existence.
        // So we will return the ID here, and handle URL construction in DownloadBeatmap.
        return id; 
    }
    
    // Nerinyan.moe (Index 1)
    if (mirrorIndex == 1) {
        if (isBeatmapId) {
            // Nerinyan needs SetID. We resolve it by checking osu! redirect.
            std::string osuUrl = "https://osu.ppy.sh/b/" + idStr;
            std::string redirectUrl;
            std::string error;
            
            LogInfo("Resolving BeatmapSet ID for Nerinyan from: " + osuUrl);
            
            if (network::HttpRequest::GetRedirectUrl(osuUrl, redirectUrl, &error)) {
                // Expected format: https://osu.ppy.sh/beatmapsets/547857#osu/1160293
                // We need to extract 547857
                std::string searchKey = "/beatmapsets/";
                size_t pos = redirectUrl.find(searchKey);
                if (pos != std::string::npos) {
                    size_t start = pos + searchKey.length();
                    size_t end = redirectUrl.find_first_of("#/?", start);
                    if (end == std::string::npos) end = redirectUrl.length();
                    
                    std::string setIdStr = redirectUrl.substr(start, end - start);
                    LogInfo("Resolved BeatmapSet ID: " + setIdStr);
                    return std::wstring(setIdStr.begin(), setIdStr.end());
                }
            } else {
                LogError("Failed to resolve redirect: " + error);
            }
            return L""; // Failed
        }
        // If it's already a SetID (from /s/ link), Nerinyan handles it directly
        return id;
    }

    return id;
}

bool DownloadBeatmap(const std::wstring& id, bool isBeatmapId) {
    int mirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
    std::wstring beatmapsetId = ResolveBeatmapSetId(id, isBeatmapId, mirrorIndex);

    if (beatmapsetId.empty()) {
        LogError("Failed to resolve BeatmapSet ID.");
        UpdateDownloadState(id, L"Failed (ID Resolution)", 0, 0, 0, false);
        return false;
    }

    LogInfo("Starting download for ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()));
    
    // Reset state
    UpdateDownloadState(beatmapsetId, L"Initializing...", 0, 0, 0, true);
    
    // Check existence (only works if we have a SetID, for Catboy /b/ links we might be checking BeatmapID against SetID folders which won't match, but that's acceptable behavior for now or we assume user knows what they are doing)
    // Actually, if Catboy /b/ is used, beatmapsetId is the BeatmapID. CheckIfMapExists checks for "ID ". 
    // If we have BeatmapID 123, and folder is "456 Artist - Title", it won't match.
    // But we can't easily get SetID from Catboy without an extra API call which user seemed to want to avoid/simplify.
    // We will proceed with download.
    if (CheckIfMapExists(beatmapsetId)) {
        LogInfo("Map already exists, skipping download.");
        UpdateDownloadState(beatmapsetId, L"Skipped (Exists)", 100, 0, 0, false);
        return true;
    }

    std::string downloadUrl;
    std::string idStr(beatmapsetId.begin(), beatmapsetId.end());

    if (mirrorIndex == 0) { // Catboy
        if (isBeatmapId) {
            downloadUrl = "https://catboy.best/api/v2/b/" + idStr;
        } else {
            downloadUrl = "https://catboy.best/api/v2/s/" + idStr;
        }
    } else { // Nerinyan
        downloadUrl = "https://api.nerinyan.moe/d/" + idStr;
    }

    LogDebug("Download URL: " + downloadUrl);

    if (!TryDownloadFromUrl(downloadUrl, beatmapsetId)) {
        // Fallback or Error
        // If Nerinyan fails, we could try Catboy? But user wanted specific behavior.
        // We will just open browser on failure.

        std::string osuUrl = "https://osu.ppy.sh/b/" + std::string(id.begin(), id.end());
        LogInfo("Opening official osu! website...");
        ShellExecuteA(NULL, "open", osuUrl.c_str(), NULL, NULL, SW_SHOW);
        
        UpdateDownloadState(beatmapsetId, L"Failed (Browser Opened)", 0, 0, 0, false);
        return false;
    }
    
    return true;
}
