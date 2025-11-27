#pragma once
#include <windows.h>

class InputHookManager {
public:
    static InputHookManager& Instance();

    void Install(HWND targetWindow);
    void Remove();
    void EnsureHooks(HWND targetWindow);

private:
    InputHookManager() = default;
    ~InputHookManager() = default;
    InputHookManager(const InputHookManager&) = delete;
    InputHookManager& operator=(const InputHookManager&) = delete;

    static LRESULT CALLBACK HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam);

    HWND m_TargetWindow = nullptr;
    WNDPROC m_OriginalWndProc = nullptr;
    HHOOK m_MessageHook = nullptr;
};
