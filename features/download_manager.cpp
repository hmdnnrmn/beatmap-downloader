// download_manager.cpp
#include "download_manager.h"
#include <iostream>
#include "utils/logging.h"
#include "network/HttpRequest.h"
#include <shlobj.h>
#include <shellapi.h>
#include "config/config_manager.h"
#include "notification_manager.h"
#include <filesystem>
#include <mutex>
#include <memory>
#include "providers/ProviderRegistry.h"
#include "providers/Resolver.h"
#include "HistoryManager.h"
#include "features/database/database.h"

namespace fs = std::filesystem;

// Global state
static DownloadState g_DownloadState;
static std::mutex g_StateMutex;
static OsuDatabase g_OsuDb;

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
    
    // Dynamic osu! root path
    wchar_t localAppData[MAX_PATH];
    std::string osuRoot;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        std::wstring ws(localAppData);
        std::string s(ws.begin(), ws.end());
        osuRoot = s + "\\osu!";
    } else {
        LogError("Failed to get LocalAppData path.");
        return false;
    }
    
    std::string dbPath = osuRoot + "\\osu!.db";
    
    g_OsuDb.Load(dbPath);

    LogInfo("Download manager initialized (Network Layer Refactored)");
    return true;
}

void CleanupDownloadManager() {
    network::HttpRequest::GlobalCleanup();
    LogInfo("Download manager cleaned up");
}

bool CheckIfMapExists(const std::wstring& beatmapId) {
    try {
        int setId = std::stoi(beatmapId);
        if (g_OsuDb.GetSetIds().count(setId)) {
            LogInfo("Beatmap Set already exists in database: " + std::to_string(setId));
            return true;
        }
    } catch (...) {
        // Not a valid integer ID, ignore
    }
    return false;
}

bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& filename, const std::wstring& beatmapId, const std::wstring& title) {
    // Dynamic download path
    wchar_t localAppData[MAX_PATH];
    std::wstring songsPath;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        songsPath = std::wstring(localAppData) + L"\\osu!\\Downloads";
    } else {
        LogError("Failed to get LocalAppData path for download.");
        return false;
    }
    std::wstring fullPath = songsPath + L"\\" + filename;

    LogDebug("Target path: " + std::string(fullPath.begin(), fullPath.end()));

    // Ensure Downloads directory exists
    if (GetFileAttributesW(songsPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(songsPath.c_str(), NULL)) {
             LogError("Downloads directory does not exist and could not be created.");
             UpdateDownloadState(beatmapId, L"Error: No Downloads Dir", 0, 0, 0, false);
             HistoryManager::Instance().AddEntry({title, beatmapId, "Failed (No Downloads Dir)", std::time(nullptr)});
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
        HistoryManager::Instance().AddEntry({title, beatmapId, "Success", std::time(nullptr)});

        if (ConfigManager::Instance().GetAutoOpen()) {
            ShellExecuteW(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_HIDE);
        }
        return true;
    } else {
        LogError("Download failed: " + error);
        UpdateDownloadState(beatmapId, L"Failed", 0, 0, 0, false);
        HistoryManager::Instance().AddEntry({title, beatmapId, "Failed", std::time(nullptr)});
        return false;
    }
}

bool DownloadBeatmap(const std::wstring& id, bool isBeatmapId, const std::wstring& artist, const std::wstring& title) {
    // 1. Resolve ID to SetID
    std::wstring beatmapsetId = id;
    if (isBeatmapId) {
        beatmapsetId = Resolver::ResolveSetIdFromBeatmapId(id);
        if (beatmapsetId.empty()) {
            LogError("Failed to resolve BeatmapSet ID from Beatmap ID: " + std::string(id.begin(), id.end()));
            UpdateDownloadState(id, L"Failed (ID Resolution)", 0, 0, 0, false);
            return false;
        }
    }

    // 2. Fetch Metadata using Metadata Mirror
    int metadataMirrorIndex = ConfigManager::Instance().GetMetadataMirrorIndex();
    // We assume index 0 is osu.direct and 1 is catboy for metadata, but we should probably check names or use a specific logic.
    // For now, let's try to get the provider by index from registry.
    // If the user selected a provider that doesn't support metadata (returns nullopt), we might fallback or fail.
    // Given the user instruction, we assume the user will select a valid metadata mirror (osu.direct or catboy).
    
    std::unique_ptr<Provider> metadataProvider = ProviderRegistry::Instance().CreateProvider(metadataMirrorIndex);
    if (!metadataProvider) {
        LogError("Invalid metadata provider index: " + std::to_string(metadataMirrorIndex));
        // Fallback to default filename if metadata fails? Or fail?
        // Let's try to proceed with default filename if metadata fails, but user asked for specific format.
    }

    std::wstring filename = beatmapsetId + L".osz";
    std::wstring finalTitle = beatmapsetId; // Default title is ID

    bool metadataAvailable = !artist.empty() && !title.empty();
    
    if (metadataAvailable) {
         // Use provided metadata
        std::wstring safeArtist = artist;
        std::wstring safeTitle = title;
        
        // Basic sanitization
        auto sanitize = [](std::wstring& s) {
            const std::wstring invalid = L"\\/:*?\"<>|";
            for (auto& c : s) {
                if (invalid.find(c) != std::wstring::npos) c = L'_';
            }
        };
        sanitize(safeArtist);
        sanitize(safeTitle);

        filename = beatmapsetId + L" " + safeArtist + L" - " + safeTitle + L".osz";
        finalTitle = safeArtist + L" - " + safeTitle;
    } else if (metadataProvider) {
        auto info = metadataProvider->GetBeatmapSetInfo(beatmapsetId);
        if (info.has_value()) {
            // Format: "{setid} Artist - Title"
            // We need to sanitize filename
            std::wstring fetchedArtist = info->artist;
            std::wstring fetchedTitle = info->title;
            
            // Basic sanitization
            auto sanitize = [](std::wstring& s) {
                const std::wstring invalid = L"\\/:*?\"<>|";
                for (auto& c : s) {
                    if (invalid.find(c) != std::wstring::npos) c = L'_';
                }
            };
            sanitize(fetchedArtist);
            sanitize(fetchedTitle);

            filename = beatmapsetId + L" " + fetchedArtist + L" - " + fetchedTitle + L".osz";
            finalTitle = fetchedArtist + L" - " + fetchedTitle;
        } else {
            LogInfo("Failed to fetch metadata using " + metadataProvider->GetName() + ", using default filename.");
        }
    }

    // 3. Download using Download Mirror
    int downloadMirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
    std::unique_ptr<Provider> downloadProvider = ProviderRegistry::Instance().CreateProvider(downloadMirrorIndex);

    if (!downloadProvider) {
        LogError("Invalid download provider index: " + std::to_string(downloadMirrorIndex));
        UpdateDownloadState(beatmapsetId, L"Failed (Invalid Provider)", 0, 0, 0, false);
        return false;
    }

    LogInfo("Starting download for ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()) + " using " + downloadProvider->GetName());
    
    // Reset state
    UpdateDownloadState(beatmapsetId, L"Initializing...", 0, 0, 0, true);
    
    if (CheckIfMapExists(beatmapsetId)) {
        LogInfo("Map already exists, skipping download.");
        UpdateDownloadState(beatmapsetId, L"Skipped (Exists)", 100, 0, 0, false);
        HistoryManager::Instance().AddEntry({finalTitle, beatmapsetId, "Skipped (Exists)", std::time(nullptr)});
        return true;
    }

    std::string downloadUrl = downloadProvider->GetDownloadUrl(beatmapsetId, false); // We already resolved to SetID
    LogDebug("Download URL: " + downloadUrl);

    if (!TryDownloadFromUrl(downloadUrl, filename, beatmapsetId, finalTitle)) {
        std::string osuUrl = "https://osu.ppy.sh/b/" + std::string(id.begin(), id.end()); // Use original ID for browser link if available
        LogInfo("Opening official osu! website...");
        ShellExecuteA(NULL, "open", osuUrl.c_str(), NULL, NULL, SW_SHOW);
        
        UpdateDownloadState(beatmapsetId, L"Failed (Browser Opened)", 0, 0, 0, false);
        return false;
    }
    
    return true;
}
