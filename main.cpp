// main.cpp - DLL Entry Point
#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include "shell_hook.h"
#include "clipboard_listener.h"
#include "download_manager.h"
#include "logging.h"

// Global variables
HMODULE g_hModule = NULL;
HANDLE g_hConsole = NULL;

// Function to allocate and show console
void AllocateConsole() {
    if (AllocConsole()) {
        // Redirect stdout, stdin, stderr to console
        FILE* pCout;
        FILE* pCin;
        FILE* pCerr;
        
        freopen_s(&pCout, "CONOUT$", "w", stdout);
        freopen_s(&pCin, "CONIN$", "r", stdin);
        freopen_s(&pCerr, "CONOUT$", "w", stderr);
        
        // Make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
        std::ios::sync_with_stdio(true);
        
        // Set console title
        SetConsoleTitle(L"osu! Beatmap Downloader");
        
        std::cout << "[INFO] Console initialized successfully!" << std::endl;
        std::cout << "[INFO] osu! Beatmap Downloader loaded" << std::endl;
    }
}

// Cleanup function
void Cleanup() {
    LogInfo("Cleaning up...");
    
    StopClipboardListener();
    UnhookShellExecute();
    CleanupDownloadManager();
    
    #ifdef _DEBUG
        if (g_hConsole) {
            FreeConsole();
        }
    #endif
    
    LogInfo("Cleanup completed");
}

// DLL Entry Point
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        
        #ifdef _DEBUG
            // Only allocate console in debug builds
            AllocateConsole();
            LogInfo("Console initialized successfully!");
        #endif
        
        // Initialize download manager
        if (!InitializeDownloadManager()) {
            std::cout << "[ERROR] Failed to initialize download manager" << std::endl;
            return FALSE;
        }
        
        // Hook ShellExecuteExW
        if (!HookShellExecute()) {
            std::cout << "[ERROR] Failed to hook ShellExecuteExW" << std::endl;
            return FALSE;
        }
        
        // Start clipboard monitoring
        if (!StartClipboardListener()) {
            std::cout << "[ERROR] Failed to start clipboard listener" << std::endl;
            return FALSE;
        }
        
        std::cout << "[INFO] DLL injected successfully!" << std::endl;
        break;
        
    case DLL_PROCESS_DETACH:
        Cleanup();
        break;
    }
    return TRUE;
}