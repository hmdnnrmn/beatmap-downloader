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
#include <memory>
#include "providers/ProviderRegistry.h"

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

bool DownloadBeatmap(const std::wstring& id, bool isBeatmapId) {
    int mirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
    std::unique_ptr<Provider> provider = ProviderRegistry::Instance().CreateProvider(mirrorIndex);

    if (!provider) {
        LogError("Invalid provider index: " + std::to_string(mirrorIndex));
        UpdateDownloadState(id, L"Failed (Invalid Provider)", 0, 0, 0, false);
        return false;
    }

    std::wstring beatmapsetId = provider->ResolveBeatmapSetId(id, isBeatmapId);

    if (beatmapsetId.empty()) {
        LogError("Failed to resolve BeatmapSet ID using provider: " + provider->GetName());
        UpdateDownloadState(id, L"Failed (ID Resolution)", 0, 0, 0, false);
        return false;
    }

    LogInfo("Starting download for ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()) + " using " + provider->GetName());
    
    // Reset state
    UpdateDownloadState(beatmapsetId, L"Initializing...", 0, 0, 0, true);
    
    if (CheckIfMapExists(beatmapsetId)) {
        LogInfo("Map already exists, skipping download.");
        UpdateDownloadState(beatmapsetId, L"Skipped (Exists)", 100, 0, 0, false);
        return true;
    }

    std::string downloadUrl = provider->GetDownloadUrl(beatmapsetId, isBeatmapId);
    LogDebug("Download URL: " + downloadUrl);

    if (!TryDownloadFromUrl(downloadUrl, beatmapsetId)) {
        std::string osuUrl = "https://osu.ppy.sh/b/" + std::string(id.begin(), id.end());
        LogInfo("Opening official osu! website...");
        ShellExecuteA(NULL, "open", osuUrl.c_str(), NULL, NULL, SW_SHOW);
        
        UpdateDownloadState(beatmapsetId, L"Failed (Browser Opened)", 0, 0, 0, false);
        return false;
    }
    
    return true;
}
