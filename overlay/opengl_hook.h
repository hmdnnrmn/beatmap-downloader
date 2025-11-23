#pragma once

#include <windows.h>
#include <GL/gl.h>
#include <string>
#include <iostream>
#include <vector>
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl3.h"
#include "input_hook.h"
#include "../download_manager.h"
#include "../config_manager.h"

static HGLRC        g_LastGLRC         = nullptr;
static HWND         g_TargetHwnd       = nullptr;
static bool         g_ImGuiInitialized = false;
static bool         g_OverlayVisible   = false;

// Message Hook
static HHOOK        g_MessageHook      = nullptr;

// Function pointers for original GL functions
typedef BOOL (WINAPI *wglSwapBuffers_t)(HDC);
static wglSwapBuffers_t g_OriginalSwapBuffers = nullptr;

// Forward declarations
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace OverlayGL {

    static void LogInfo(const std::string& msg) {
        std::cout << "[OverlayGL] " << msg << std::endl;
    }

    static void LogError(const std::string& msg) {
        std::cerr << "[OverlayGL] ERROR: " << msg << std::endl;
    }

    static void UpdateRawInputState() {
        if (g_OverlayVisible) {
            InputHook::DisableRawInputDevices();
        } else {
            InputHook::RestoreRawInputDevices();
        }
    }

    // Global Message Hook Procedure
    static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode < 0) // Do not process message 
            return CallNextHookEx(g_MessageHook, nCode, wParam, lParam);

        if (nCode == HC_ACTION && wParam == PM_REMOVE) {
            MSG* msg = (MSG*)lParam;

            // Handle Home key to toggle overlay
            if (msg->message == WM_KEYDOWN && msg->wParam == VK_HOME) {
                g_OverlayVisible = !g_OverlayVisible;
                UpdateRawInputState();
                LogInfo("Overlay toggled via Home key.");
            }

            if (g_ImGuiInitialized) {
                // Pass input to ImGui
                if (g_OverlayVisible) {
                    ImGui_ImplWin32_WndProcHandler(msg->hwnd, msg->message, msg->wParam, msg->lParam);
                }

                // Block input to game if overlay is visible
                if (g_OverlayVisible) {
                    switch (msg->message) {
                        case WM_KEYDOWN:
                        case WM_KEYUP:
                        case WM_SYSKEYDOWN:
                        case WM_SYSKEYUP:
                        case WM_CHAR:
                        case WM_MOUSEMOVE:
                        case WM_LBUTTONDOWN:
                        case WM_LBUTTONUP:
                        case WM_LBUTTONDBLCLK:
                        case WM_RBUTTONDOWN:
                        case WM_RBUTTONUP:
                        case WM_RBUTTONDBLCLK:
                        case WM_MBUTTONDOWN:
                        case WM_MBUTTONUP:
                        case WM_MBUTTONDBLCLK:
                        case WM_MOUSEWHEEL:
                        case WM_XBUTTONDOWN:
                        case WM_XBUTTONUP:
                        case WM_XBUTTONDBLCLK:
                            // Nullify the message to block it
                            msg->message = WM_NULL;
                            return 0; 
                    }
                }
            }
        }

        return CallNextHookEx(g_MessageHook, nCode, wParam, lParam);
    }

    static void RenderOverlayFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Enable software cursor if overlay is visible
        ImGui::GetIO().MouseDrawCursor = g_OverlayVisible;

        if (g_OverlayVisible) {
            ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("osu! Beatmap Downloader", &g_OverlayVisible)) {
                
                if (ImGui::BeginTabBar("MainTabs")) {
                    if (ImGui::BeginTabItem("Status")) {
                        DownloadState state = GetDownloadState();
                        if (state.isDownloading) {
                            std::string filenameStr(state.filename.begin(), state.filename.end());
                            ImGui::Text("Downloading: %s", filenameStr.c_str());
                            ImGui::ProgressBar(state.progress / 100.0f, ImVec2(-1, 0), 
                                std::to_string((int)state.progress).c_str());
                            ImGui::Text("%.2f MB / %.2f MB", 
                                state.downloadedBytes / (1024.0f * 1024.0f), 
                                state.totalBytes / (1024.0f * 1024.0f));
                        } else {
                            ImGui::Text("Idle. Waiting for beatmap link...");
                        }
                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Settings")) {
                        static char songsPath[256] = "";
                        static int mirrorIndex = 0;
                        static bool autoOpen = false;
                        static bool clipboardEnabled = true;
                        static bool initSettings = false;

                        if (!initSettings) {
                            std::wstring wPath = ConfigManager::Instance().GetSongsPath();
                            std::string sPath(wPath.begin(), wPath.end());
                            strcpy_s(songsPath, sPath.c_str());
                            
                            mirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
                            autoOpen = ConfigManager::Instance().GetAutoOpen();
                            clipboardEnabled = ConfigManager::Instance().IsClipboardEnabled();
                            initSettings = true;
                        }

                        if (ImGui::InputText("Songs Path", songsPath, IM_ARRAYSIZE(songsPath))) {
                            std::string sPath = songsPath;
                            std::wstring wPath(sPath.begin(), sPath.end());
                            ConfigManager::Instance().SetSongsPath(wPath);
                        }
                        
                        const char* mirrors[] = { "Catboy.best", "Nerinyan.moe" };
                        if (ImGui::Combo("Mirror", &mirrorIndex, mirrors, IM_ARRAYSIZE(mirrors))) {
                            ConfigManager::Instance().SetDownloadMirrorIndex(mirrorIndex);
                        }

                        if (ImGui::Checkbox("Auto Open (.osz)", &autoOpen)) {
                            ConfigManager::Instance().SetAutoOpen(autoOpen);
                        }

                        if (ImGui::Checkbox("Enable Clipboard Listener", &clipboardEnabled)) {
                            ConfigManager::Instance().SetClipboardEnabled(clipboardEnabled);
                        }

                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
            }
            ImGui::End();
        }

        // Non-blocking Download Popup (Always visible if downloading)
        DownloadState state = GetDownloadState();
        if (state.isDownloading && !g_OverlayVisible) {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, ImGui::GetIO().DisplaySize.y - 80), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.6f); // Semi-transparent
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | 
                                     ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoFocusOnAppearing;
            
            if (ImGui::Begin("DownloadPopup", nullptr, flags)) {
                std::string filenameStr(state.filename.begin(), state.filename.end());
                ImGui::Text("Downloading: %s", filenameStr.c_str());
                ImGui::ProgressBar(state.progress / 100.0f, ImVec2(-1, 0));
            }
            ImGui::End();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    static void InitializeImGuiIfNeeded(HDC hdc) {
        if (g_ImGuiInitialized) return;

        HWND hwnd = WindowFromDC(hdc);
        if (!hwnd) return;

        // Install Message Hook
        if (!g_MessageHook) {
            DWORD threadId = GetCurrentThreadId();
            g_MessageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, threadId);
            if (g_MessageHook) {
                LogInfo("Message hook installed successfully.");
            } else {
                LogError("Failed to install message hook.");
            }
        }

        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        if (!ImGui_ImplWin32_Init(hwnd)) {
            LogError("ImGui_ImplWin32_Init failed.");
            return;
        }
        
        if (!ImGui_ImplOpenGL3_Init()) { // Auto-detects GL version
             LogError("ImGui_ImplOpenGL3_Init failed.");
             ImGui_ImplWin32_Shutdown();
             return;
        }

        g_TargetHwnd       = hwnd;
        g_LastGLRC         = wglGetCurrentContext();
        g_ImGuiInitialized = true;
        
        // Initial state check
        UpdateRawInputState();

        LogInfo("ImGui initialized for OpenGL3 + Win32.");
    }

    static void ShutdownImGui() {
        if (!g_ImGuiInitialized) return;

        // Uninstall Message Hook
        if (g_MessageHook) {
            UnhookWindowsHookEx(g_MessageHook);
            g_MessageHook = nullptr;
            LogInfo("Message hook uninstalled.");
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        
        InputHook::RestoreRawInputDevices();

        g_ImGuiInitialized = false;
        LogInfo("ImGui shutdown.");
    }

    static void RebindToWindowIfChanged(HDC hdc) {
        HWND currentHwnd = WindowFromDC(hdc);
        HGLRC currentCtx = wglGetCurrentContext();

        if (!currentHwnd) return;

        // Check if window or context changed
        bool contextChanged = (currentHwnd != g_TargetHwnd || currentCtx != g_LastGLRC);

        if (contextChanged) {
            LogInfo("Context or Window changed, re-initializing ImGui...");
            ShutdownImGui();
            InitializeImGuiIfNeeded(hdc);
        }
    }

    static void PatchSwapBuffers();
    static void UnpatchSwapBuffers();

    static BOOL WINAPI HookedSwapBuffers(HDC hdc) {
        // Initialize ImGui once we have a valid context and window
        if (!g_ImGuiInitialized) {
            InitializeImGuiIfNeeded(hdc);
        } else {
            RebindToWindowIfChanged(hdc);
        }

        // Render overlay
        if (g_ImGuiInitialized) {
            RenderOverlayFrame();
        }

        // Temporarily unhook to call the original
        UnpatchSwapBuffers();
        BOOL result = g_OriginalSwapBuffers ? g_OriginalSwapBuffers(hdc) : FALSE;
        PatchSwapBuffers();

        return result;
    }

    // Simple JMP detour for SwapBuffers (x86)
    static unsigned char g_OriginalBytes[5] = {0};
    static bool g_IsHooked = false;
    static CRITICAL_SECTION g_HookCS;
    static bool g_HookCSInit = false;

    // Helper for memory writing
    static bool WriteMemory(void* address, const void* data, size_t size) {
        DWORD oldProtect;
        if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) return false;
        memcpy(address, data, size);
        VirtualProtect(address, size, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), address, size);
        return true;
    }

    static void PatchSwapBuffers() {
        if (g_IsHooked) return;
        if (!g_HookCSInit) return; // Should be init by Install
        
        EnterCriticalSection(&g_HookCS);

        HMODULE hGdi32 = GetModuleHandleA("gdi32.dll");
        if (!hGdi32) { LeaveCriticalSection(&g_HookCS); return; }

        void* pSwapBuffers = (void*)GetProcAddress(hGdi32, "SwapBuffers");
        if (!pSwapBuffers) { LeaveCriticalSection(&g_HookCS); return; }

        if (!g_OriginalSwapBuffers) {
            g_OriginalSwapBuffers = (wglSwapBuffers_t)pSwapBuffers;
        }

        // Save original bytes once
        static bool firstTime = true;
        if (firstTime) {
            memcpy(g_OriginalBytes, pSwapBuffers, 5);
            firstTime = false;
        }

        // JMP rel32
        uintptr_t relAddr = (uintptr_t)HookedSwapBuffers - ((uintptr_t)pSwapBuffers + 5);
        unsigned char patch[5];
        patch[0] = 0xE9;
        memcpy(&patch[1], &relAddr, 4);

        WriteMemory(pSwapBuffers, patch, 5);
        
        g_IsHooked = true;
        LeaveCriticalSection(&g_HookCS);
    }

    static void UnpatchSwapBuffers() {
        if (!g_IsHooked) return;
        if (!g_HookCSInit) return;

        EnterCriticalSection(&g_HookCS);

        HMODULE hGdi32 = GetModuleHandleA("gdi32.dll");
        if (hGdi32) {
            void* pSwapBuffers = (void*)GetProcAddress(hGdi32, "SwapBuffers");
            if (pSwapBuffers) {
                WriteMemory(pSwapBuffers, g_OriginalBytes, 5);
            }
        }

        g_IsHooked = false;
        LeaveCriticalSection(&g_HookCS);
    }

    // Public API
    inline bool InstallOpenGLHooks() {
        if (!g_HookCSInit) {
            InitializeCriticalSection(&g_HookCS);
            g_HookCSInit = true;
        }

        PatchSwapBuffers();
        return true;
    }

    inline void RemoveOpenGLHooks() {
        UnpatchSwapBuffers();
        ShutdownImGui();
        if (g_HookCSInit) {
            DeleteCriticalSection(&g_HookCS);
            g_HookCSInit = false;
        }
    }

    inline bool IsOverlayVisible() {
        return g_OverlayVisible;
    }

    inline void SetOverlayVisible(bool visible) {
        g_OverlayVisible = visible;
        UpdateRawInputState();
    }

} // namespace OverlayGL