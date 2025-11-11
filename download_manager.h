// download_manager.h
#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <windows.h>
#include <string>

bool InitializeDownloadManager();
void CleanupDownloadManager();
bool DownloadBeatmap(const std::wstring& beatmapId);
bool TryDownloadFromUrl(const std::string& downloadUrl, const std::wstring& beatmapId);
void CheckClipboardForBeatmapLinks();

#endif