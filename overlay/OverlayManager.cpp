#include "OverlayManager.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl3.h"
#include "input_hook.h"
#include "tabs/TabManager.h"
#include "tabs/TabRegistry.h"
#include "features/download_manager.h"
#include <iostream>

OverlayManager& OverlayManager::Instance() {
    static OverlayManager instance;
    return instance;
}

void OverlayManager::Initialize(HDC hdc) {
    if (m_Initialized) return;

    HWND hwnd = WindowFromDC(hdc);
    if (!hwnd) return;

    // Ensure we get the main window
    m_TargetWindow = GetAncestor(hwnd, GA_ROOT);
    if (!m_TargetWindow) m_TargetWindow = hwnd;

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    if (!ImGui_ImplWin32_Init(m_TargetWindow)) {
        std::cerr << "[OverlayManager] ImGui_ImplWin32_Init failed." << std::endl;
        return;
    }
    
    if (!ImGui_ImplOpenGL3_Init()) {
         std::cerr << "[OverlayManager] ImGui_ImplOpenGL3_Init failed." << std::endl;
         ImGui_ImplWin32_Shutdown();
         return;
    }

    m_LastContext = wglGetCurrentContext();
    m_Initialized = true;
    
    UpdateRawInput();
    TabRegistry::RegisterAllTabs();

    std::cout << "[OverlayManager] Initialized." << std::endl;
}

void OverlayManager::Shutdown() {
    if (!m_Initialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    
    InputHook::RestoreRawInputDevices();

    m_Initialized = false;
    m_TargetWindow = nullptr;
    std::cout << "[OverlayManager] Shutdown." << std::endl;
}

void OverlayManager::Render() {
    if (!m_Initialized) return;

    // Check for context change
    if (wglGetCurrentContext() != m_LastContext) {
        // Handle context change if necessary, or just update tracker
        m_LastContext = wglGetCurrentContext();
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::GetIO().MouseDrawCursor = m_Visible;

    if (m_Visible) {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowFocus(); // Force focus to the next window
        if (ImGui::Begin("osu! Beatmap Downloader", &m_Visible)) {
            TabManager::Instance().Render();
        }
        ImGui::End();
    }

    // Non-blocking Download Popup
    DownloadState state = GetDownloadState();
    if (state.isDownloading && !m_Visible) {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 320, ImGui::GetIO().DisplaySize.y - 80), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.6f);
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
    
    // If visibility changed during render (e.g. user clicked close button), update raw input
    UpdateRawInput();
}

void OverlayManager::SetVisible(bool visible) {
    if (m_Visible != visible) {
        m_Visible = visible;
        UpdateRawInput();
    }
}

void OverlayManager::ToggleVisibility() {
    SetVisible(!m_Visible);
}

void OverlayManager::UpdateRawInput() {
    if (m_Visible) {
        InputHook::DisableRawInputDevices();
    } else {
        InputHook::RestoreRawInputDevices();
    }
}
