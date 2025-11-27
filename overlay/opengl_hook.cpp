#include "opengl_hook.h"
#include <iostream>
#include "OverlayManager.h"
#include "InputHookManager.h"

// Function pointers for original GL functions
typedef BOOL (WINAPI *wglSwapBuffers_t)(HDC);
static wglSwapBuffers_t g_OriginalSwapBuffers = nullptr;

namespace OverlayGL {

    static void PatchSwapBuffers();
    static void UnpatchSwapBuffers();

    static BOOL WINAPI HookedSwapBuffers(HDC hdc) {
        // Initialize Overlay
        if (!OverlayManager::Instance().IsInitialized()) {
            OverlayManager::Instance().Initialize(hdc);
            
            // Install Input Hooks once overlay is initialized
            HWND target = OverlayManager::Instance().GetTargetWindow();
            if (target) {
                InputHookManager::Instance().Install(target);
            }
        } else {
            // Ensure hooks are active
            HWND target = OverlayManager::Instance().GetTargetWindow();
            if (target) {
                InputHookManager::Instance().EnsureHooks(target);
            }
        }

        // Render Overlay
        OverlayManager::Instance().Render();

        // Call Original SwapBuffers
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
        if (!g_HookCSInit) return; 
        
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
    bool InstallOpenGLHooks() {
        if (!g_HookCSInit) {
            InitializeCriticalSection(&g_HookCS);
            g_HookCSInit = true;
        }

        PatchSwapBuffers();
        return true;
    }

    void RemoveOpenGLHooks() {
        UnpatchSwapBuffers();
        InputHookManager::Instance().Remove();
        OverlayManager::Instance().Shutdown();
        
        if (g_HookCSInit) {
            DeleteCriticalSection(&g_HookCS);
            g_HookCSInit = false;
        }
    }

    bool IsOverlayVisible() {
        return OverlayManager::Instance().IsVisible();
    }

    void SetOverlayVisible(bool visible) {
        OverlayManager::Instance().SetVisible(visible);
    }

    // C-style exports for main.cpp usage
    extern "C" {
        __declspec(dllexport) BOOL Overlay_InstallOpenGLHooks() {
            return InstallOpenGLHooks() ? TRUE : FALSE;
        }
        __declspec(dllexport) void Overlay_RemoveOpenGLHooks() {
            RemoveOpenGLHooks();
        }
        __declspec(dllexport) void Overlay_SetVisible(BOOL visible) {
            SetOverlayVisible(visible ? true : false);
        }
        __declspec(dllexport) BOOL Overlay_IsVisible() {
            return IsOverlayVisible() ? TRUE : FALSE;
        }
    }

} // namespace OverlayGL