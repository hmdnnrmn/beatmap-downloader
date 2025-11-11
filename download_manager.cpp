// download_manager.cpp
#include "download_manager.h"
#include <iostream>
#include "logging.h"
#include <curl/curl.h>
#include <curl/easy.h> 
#include <shlobj.h>
#include <shellapi.h>
#include <fstream>

#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "shell32.lib")

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

bool DownloadBeatmap(const std::wstring& beatmapId) {
    LogInfo("Starting download for beatmap ID: " + std::string(beatmapId.begin(), beatmapId.end()));

    // Try catboy.best first
    std::string downloadUrl = "https://catboy.best/d/" + std::string(beatmapId.begin(), beatmapId.end());
    LogDebug("Trying primary download URL: " + downloadUrl);

    if (!TryDownloadFromUrl(downloadUrl, beatmapId)) {
        // If catboy.best fails, try nerinyan.moe
        downloadUrl = "https://api.nerinyan.moe/d/" + std::string(beatmapId.begin(), beatmapId.end());
        LogDebug("Trying secondary download URL: " + downloadUrl);

        if (!TryDownloadFromUrl(downloadUrl, beatmapId)) {
            // If both downloads fail, open official osu! website
            std::string osuUrl = "https://osu.ppy.sh/beatmapsets/" + std::string(beatmapId.begin(), beatmapId.end());
            LogInfo("Opening official osu! website...");
            ShellExecuteA(NULL, "open", osuUrl.c_str(), NULL, NULL, SW_SHOW);
            return false;
        }
    }
    
    return true;
}

bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& beatmapId) {
    // Get osu! Songs folder path
    wchar_t documentsPath[MAX_PATH];
    if (FAILED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documentsPath))) {
        LogError("Failed to get Documents folder path");
        return false;
    }
    
    std::wstring osuPath = std::wstring(documentsPath) + L"\\osu!";
    std::wstring songsPath = osuPath + L"\\Songs";
    std::wstring filename = beatmapId + L".osz";
    std::wstring fullPath = songsPath + L"\\" + filename;

    LogDebug("Target path: " + std::string(fullPath.begin(), fullPath.end()));

    // Create osu! directory first
    BOOL osuDirResult = CreateDirectory(osuPath.c_str(), NULL);
    DWORD osuDirError = GetLastError();
    if (!osuDirResult && osuDirError != ERROR_ALREADY_EXISTS) {
        LogWarning("Could not create osu! directory. Error: " + std::to_string(osuDirError));
        LogInfo("Attempting to create in Desktop folder instead...");

        // Fallback to Desktop folder
        wchar_t desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, desktopPath))) {
            songsPath = std::wstring(desktopPath);
            fullPath = songsPath + L"\\" + filename;
            LogInfo("Using Desktop folder: " + std::string(fullPath.begin(), fullPath.end()));
        } else {
            LogError("Could not find suitable download location");
            return false;
        }
    } else {
        // Create Songs directory
        BOOL songsDirResult = CreateDirectory(songsPath.c_str(), NULL);
        DWORD songsDirError = GetLastError();
        if (!songsDirResult && songsDirError != ERROR_ALREADY_EXISTS) {
            LogError("Failed to create Songs directory. Error: " + std::to_string(songsDirError));
            // Fallback to just the osu! directory
            fullPath = osuPath + L"\\" + filename;
            LogInfo("Downloading to osu! root folder: " + std::string(fullPath.begin(), fullPath.end()));
        }
    }
    
    // Check if file already exists
    if (GetFileAttributes(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        LogInfo("File already exists, skipping download");
        ShellExecute(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_HIDE);
        return true;
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

            // Open the downloaded file to import it into osu!
            ShellExecute(NULL, L"open", fullPath.c_str(), NULL, NULL, SW_HIDE);
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