// main.cpp - DLL Entry Point
#include <windows.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>

#include "hooks/shell_hook.h"

#include "hooks/clipboard_listener.h"

#include "features/download_manager.h"

#include "utils/logging.h"

#include "config/config_manager.h"

#include "features/notification_manager.h"

#include "features/download_queue.h"
#include "overlay/opengl_hook.h"


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
        OverlayGL::RemoveOpenGLHooks();

    
    DownloadQueue::Instance().Stop();
    NotificationManager::Instance().Cleanup();
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
        
        // Initialize Config
        ConfigManager::Instance().LoadConfig();

        // Initialize Notifications
        if (!NotificationManager::Instance().Initialize()) {
            std::cout << "[ERROR] Failed to initialize notification manager" << std::endl;
        }

        // Initialize download manager
        if (!InitializeDownloadManager()) {
            std::cout << "[ERROR] Failed to initialize download manager" << std::endl;
            return FALSE;
        }
        
        // Start Download Queue
        DownloadQueue::Instance().Start();

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
        

                // Install OpenGL overlay hook (non-fatal if it fails; osu! is 32-bit OpenGL)
                if (!OverlayGL::InstallOpenGLHooks()) {
                    std::cout << "[ERROR] Failed to install OpenGL overlay hooks" << std::endl;
                }
                
                std::cout << "[INFO] DLL injected successfully!" << std::endl;

        NotificationManager::Instance().ShowNotification(L"Loaded", L"osu! Beatmap Downloader is running");
        break;
        
    case DLL_PROCESS_DETACH:
        Cleanup();
        break;
    }
    return TRUE;
}