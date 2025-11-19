// download_manager.cpp
#include "download_manager.h"
#include <iostream>
#include "logging.h"
#include <curl/curl.h>
#include <curl/easy.h> 
#include <shlobj.h>
#include <shellapi.h>
#include <fstream>
#include "config_manager.h"
#include "notification_manager.h"
#include <filesystem>

// Removed pragma comments to allow project settings to control linking

namespace fs = std::filesystem;

// Structure to hold download data
struct DownloadData {
    std::ofstream* file;
    std::wstring filename;
    size_t totalSize;
    size_t downloadedSize;
};

// Progress callback function
int ProgressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    DownloadData* data = static_cast<DownloadData*>(clientp);
    
    if (dltotal > 0) {
        int progress = static_cast<int>((dlnow * 100) / dltotal);
        std::wcout << L"\r[DOWNLOAD] " << data->filename << L" - " << progress << L"% (" 
                   << dlnow << L"/" << dltotal << L" bytes)";
        std::wcout.flush();
    }
    
    return 0; // Return 0 to continue download
}

// Write callback function
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    DownloadData* data = static_cast<DownloadData*>(userp);
    size_t totalSize = size * nmemb;
    
    if (data->file && data->file->is_open()) {
        data->file->write(static_cast<const char*>(contents), totalSize);
        data->downloadedSize += totalSize;
        return totalSize;
    }
    
    return 0;
}

bool InitializeDownloadManager() {
    // Initialize libcurl globally
    CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (result != CURLE_OK) {
        LogError("Failed to initialize libcurl: " + std::string(curl_easy_strerror(result)));
        return false;
    }

    LogInfo("Download manager initialized with libcurl");
    LogInfo("libcurl version: " + std::string(curl_version()));
    return true;
}

void CleanupDownloadManager() {
    curl_global_cleanup();
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

bool DownloadBeatmap(const std::wstring& beatmapId) {
    LogInfo("Starting download for beatmap ID: " + std::string(beatmapId.begin(), beatmapId.end()));
    NotificationManager::Instance().ShowNotification(L"Downloading...", L"Beatmap ID: " + beatmapId);

    if (CheckIfMapExists(beatmapId)) {
        LogInfo("Map already exists, skipping download.");
        NotificationManager::Instance().ShowNotification(L"Skipped", L"Beatmap already exists.");
        return true;
    }

    std::string mirror = ConfigManager::Instance().GetDownloadMirror();
    std::string downloadUrl = mirror + std::string(beatmapId.begin(), beatmapId.end());
    LogDebug("Trying download URL: " + downloadUrl);

    if (!TryDownloadFromUrl(downloadUrl, beatmapId)) {
        // Fallback to nerinyan if primary fails
        if (mirror.find("catboy") != std::string::npos) {
             downloadUrl = "https://api.nerinyan.moe/d/" + std::string(beatmapId.begin(), beatmapId.end());
             LogDebug("Trying secondary download URL: " + downloadUrl);
             if (TryDownloadFromUrl(downloadUrl, beatmapId)) {
                 return true;
             }
        }

        // If downloads fail, open official osu! website
        std::string osuUrl = "https://osu.ppy.sh/beatmapsets/" + std::string(beatmapId.begin(), beatmapId.end());
        LogInfo("Opening official osu! website...");
        ShellExecuteA(NULL, "open", osuUrl.c_str(), NULL, NULL, SW_SHOW);
        NotificationManager::Instance().ShowNotification(L"Failed", L"Opening browser...");
        return false;
    }
    
    return true;
}

bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& beatmapId) {
    std::wstring songsPath = ConfigManager::Instance().GetSongsPath();
    std::wstring filename = beatmapId + L".osz";
    std::wstring fullPath = songsPath + L"\\" + filename;

    LogDebug("Target path: " + std::string(fullPath.begin(), fullPath.end()));

    // Ensure Songs directory exists
    if (GetFileAttributesW(songsPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        // Try to create it, but it might fail if parent dirs don't exist
        // For now, we assume the user has set a valid path in config or default exists
        // If default fails, we might want to fallback to Desktop, but ConfigManager should handle valid paths
        if (!CreateDirectoryW(songsPath.c_str(), NULL)) {
             LogError("Songs directory does not exist and could not be created.");
             return false;
        }
    }
    
    // Initialize CURL handle
    CURL* curl = curl_easy_init();
    if (!curl) {
        LogError("Failed to initialize CURL handle");
        return false;
    }
    
    // Open output file
    std::ofstream outFile(fullPath, std::ios::binary);
    if (!outFile.is_open()) {
        LogError("Failed to create output file: " + std::string(fullPath.begin(), fullPath.end()));
        curl_easy_cleanup(curl);
        return false;
    }
    
    // Setup download data
    DownloadData downloadData;
    downloadData.file = &outFile;
    downloadData.filename = filename;
    downloadData.totalSize = 0;
    downloadData.downloadedSize = 0;
    
    // Configure CURL options
    curl_easy_setopt(curl, CURLOPT_URL, downloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &downloadData);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, ProgressCallback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &downloadData);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "osu! Beatmap Downloader/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L); // 5 minutes timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); // 30 seconds connection timeout
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    LogInfo("Starting download from: " + std::string(downloadUrl.begin(), downloadUrl.end()));

    // Perform the download
    CURLcode res = curl_easy_perform(curl);
    
    // Close the file
    outFile.close();
    
    if (res == CURLE_OK) {
        // Get response code
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        // Get download info
        double downloaded;
        curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &downloaded);
        
        if (response_code == 200 && downloaded > 0) {
            LogInfo("Successfully downloaded: " + std::string(filename.begin(), filename.end()) +
                     " (Size: " + std::to_string(static_cast<long>(downloaded)) + " bytes)");

            NotificationManager::Instance().ShowNotification(L"Download Complete", filename);

            // Open the downloaded file to import it into osu! if enabled
            if (ConfigManager::Instance().GetAutoOpen()) {
                ShellExecute(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_HIDE);
            }
            curl_easy_cleanup(curl);
            return true;
        }

        LogError("Download failed - HTTP response code: " + std::to_string(response_code) +
                 ", Downloaded: " + std::to_string(static_cast<long>(downloaded)) + " bytes");
        DeleteFile(fullPath.c_str());
    } else {
        LogError("libcurl error: " + std::string(curl_easy_strerror(res)));
        DeleteFile(fullPath.c_str());
    }
    
    curl_easy_cleanup(curl);
    return false;
}