#pragma once
#include <windows.h>
#include <string>
#include "imgui.h"

class OverlayManager {
public:
    static OverlayManager& Instance();

    void Initialize(HDC hdc);
    void Shutdown();
    void Render();
    
    bool IsVisible() const { return m_Visible; }
    void SetVisible(bool visible);
    void ToggleVisibility();

    bool IsInitialized() const { return m_Initialized; }
    HWND GetTargetWindow() const { return m_TargetWindow; }

private:
    OverlayManager() = default;
    ~OverlayManager() = default;
    OverlayManager(const OverlayManager&) = delete;
    OverlayManager& operator=(const OverlayManager&) = delete;

    void UpdateRawInput();

    bool m_Visible = false;
    bool m_Initialized = false;
    HWND m_TargetWindow = nullptr;
    HGLRC m_LastContext = nullptr;
};
