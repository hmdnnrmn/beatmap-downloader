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

bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& filename, const std::wstring& beatmapId, const std::wstring& title) {
    std::wstring songsPath = ConfigManager::Instance().GetSongsPath();
    std::wstring fullPath = songsPath + L"\\" + filename;

    LogDebug("Target path: " + std::string(fullPath.begin(), fullPath.end()));

    // Ensure Songs directory exists
    if (GetFileAttributesW(songsPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        if (!CreateDirectoryW(songsPath.c_str(), NULL)) {
             LogError("Songs directory does not exist and could not be created.");
             UpdateDownloadState(beatmapId, L"Error: No Songs Dir", 0, 0, 0, false);
             HistoryManager::Instance().AddEntry({title, beatmapId, "Failed (No Songs Dir)", std::time(nullptr)});
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
