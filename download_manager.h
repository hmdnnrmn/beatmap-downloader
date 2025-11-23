// download_manager.h
#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <windows.h>
#include <string>

struct DownloadState {
    std::wstring beatmapId;
    std::wstring filename;
    int progress;
    size_t downloadedBytes;
    size_t totalBytes;
    bool isDownloading;
};

DownloadState GetDownloadState();

bool InitializeDownloadManager();
void CleanupDownloadManager();
bool CheckIfMapExists(const std::wstring& beatmapId);
bool DownloadBeatmap(const std::wstring& id, bool isBeatmapId = false);
bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& beatmapId);
void CheckClipboardForBeatmapLinks();

#endif