// clipboard_listener.cpp
#include "clipboard_listener.h"
#include "download_manager.h"
#include "logging.h"
#include <iostream>
#include <string>
#include <regex>
#include <thread>

// Global variables for clipboard monitoring
HWND g_hClipboardWnd = NULL;
HWND g_hNextClipboardViewer = NULL;
bool g_bClipboardListening = false;
std::thread g_clipboardThread;
std::wstring GetBeatmapsetId(const std::wstring& beatmapId);

// Window procedure for clipboard monitoring
LRESULT CALLBACK ClipboardWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DRAWCLIPBOARD:
        // Clipboard content changed
        if (g_bClipboardListening) {
            CheckClipboardForBeatmapLinks();
        }
        
        // Pass message to next viewer
        if (g_hNextClipboardViewer != NULL) {
            SendMessage(g_hNextClipboardViewer, uMsg, wParam, lParam);
        }
        break;
        
    case WM_CHANGECBCHAIN:
        // Clipboard viewer chain changed
        if ((HWND)wParam == g_hNextClipboardViewer) {
            g_hNextClipboardViewer = (HWND)lParam;
        } else if (g_hNextClipboardViewer != NULL) {
            SendMessage(g_hNextClipboardViewer, uMsg, wParam, lParam);
        }
        break;
        
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void ClipboardMonitorThread() {
    // Register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc = ClipboardWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"OsuBeatmapClipboardListener";
    
    RegisterClass(&wc);
    
    // Create invisible window for clipboard monitoring
    g_hClipboardWnd = CreateWindow(
        L"OsuBeatmapClipboardListener",
        L"Clipboard Listener",
        0, 0, 0, 0, 0,
        HWND_MESSAGE,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!g_hClipboardWnd) {
        LogError("Failed to create clipboard window");
        return;
    }
    
    // Add to clipboard viewer chain
    g_hNextClipboardViewer = SetClipboardViewer(g_hClipboardWnd);

    LogInfo("Clipboard listener started");

    // Message loop
    MSG msg;
    while (g_bClipboardListening && GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // Clean up
    if (g_hClipboardWnd) {
        ChangeClipboardChain(g_hClipboardWnd, g_hNextClipboardViewer);
        DestroyWindow(g_hClipboardWnd);
    }

    LogInfo("Clipboard listener stopped");
}

void CheckClipboardForBeatmapLinks() {
    if (!OpenClipboard(NULL)) {
        return;
    }
    
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (hData != NULL) {
        wchar_t* pszText = static_cast<wchar_t*>(GlobalLock(hData));
        if (pszText != NULL) {
            std::wstring clipboardText(pszText);
            
            std::wstring beatmapsetId;
            
            // Check for osu! beatmapset link first
            std::wregex beatmapsetRegex(L"https://osu\\.ppy\\.sh/beatmapsets/(\\d+)");
            std::wsmatch matches;
            
            if (std::regex_search(clipboardText, matches, beatmapsetRegex)) {
                beatmapsetId = matches[1].str();
                LogInfo("Beatmapset link detected in clipboard, ID: " + std::string(beatmapsetId.begin(), beatmapsetId.end()));
            } else {
                // Check for individual beatmap link
                std::wregex beatmapRegex(L"https://osu\\.ppy\\.sh/beatmaps/(\\d+)");
                if (std::regex_search(clipboardText, matches, beatmapRegex)) {
                    std::wstring beatmapId = matches[1].str();
                    LogInfo("Beatmap link detected in clipboard, ID: " + std::string(beatmapId.begin(), beatmapId.end()));

                    // Convert beatmap ID to beatmapset ID
                    beatmapsetId = GetBeatmapsetId(beatmapId);
                    if (beatmapsetId.empty()) {
                        LogError("Failed to convert beatmap ID to beatmapset ID from clipboard");
                        GlobalUnlock(hData);
                        CloseClipboard();
                        return;
                    }
                }
            }
            
            if (!beatmapsetId.empty()) {
                // Download the beatmap using beatmapset ID
                DownloadBeatmap(beatmapsetId);
            }
            
            GlobalUnlock(hData);
        }
    }
    
    CloseClipboard();
}

bool StartClipboardListener() {
    if (g_bClipboardListening) {
        return true; // Already listening
    }
    
    g_bClipboardListening = true;
    g_clipboardThread = std::thread(ClipboardMonitorThread);
    
    return true;
}

void StopClipboardListener() {
    if (!g_bClipboardListening) {
        return;
    }
    
    g_bClipboardListening = false;
    
    // Post quit message to thread
    if (g_hClipboardWnd) {
        PostMessage(g_hClipboardWnd, WM_QUIT, 0, 0);
    }
    
    // Wait for thread to finish
    if (g_clipboardThread.joinable()) {
        g_clipboardThread.join();
    }
}