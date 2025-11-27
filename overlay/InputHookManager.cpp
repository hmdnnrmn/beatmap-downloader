#include "InputHookManager.h"
#include "OverlayManager.h"
#include "backends/imgui_impl_win32.h"
#include <iostream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

InputHookManager& InputHookManager::Instance() {
    static InputHookManager instance;
    return instance;
}

void InputHookManager::Install(HWND targetWindow) {
    if (m_TargetWindow && m_TargetWindow != targetWindow) {
        Remove();
    }
    m_TargetWindow = targetWindow;

    // Install Message Hook
    if (!m_MessageHook) {
        DWORD threadId = GetWindowThreadProcessId(targetWindow, NULL);
        m_MessageHook = SetWindowsHookEx(WH_GETMESSAGE, GetMsgProc, NULL, threadId);
        if (m_MessageHook) {
            std::cout << "[InputHookManager] Message hook installed." << std::endl;
        } else {
            std::cerr << "[InputHookManager] Failed to install message hook." << std::endl;
        }
    }

    // Subclass WndProc
    if (!m_OriginalWndProc) {
        m_OriginalWndProc = (WNDPROC)SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
        if (m_OriginalWndProc) {
            std::cout << "[InputHookManager] WndProc subclassed." << std::endl;
        } else {
            std::cerr << "[InputHookManager] Failed to subclass WndProc." << std::endl;
        }
    }
}

void InputHookManager::Remove() {
    if (m_MessageHook) {
        UnhookWindowsHookEx(m_MessageHook);
        m_MessageHook = nullptr;
    }

    if (m_OriginalWndProc && m_TargetWindow) {
        SetWindowLongPtr(m_TargetWindow, GWLP_WNDPROC, (LONG_PTR)m_OriginalWndProc);
        m_OriginalWndProc = nullptr;
    }
    m_TargetWindow = nullptr;
}

void InputHookManager::EnsureHooks(HWND targetWindow) {
    if (!targetWindow) return;
    
    // Check WndProc
    WNDPROC currentWndProc = (WNDPROC)GetWindowLongPtr(targetWindow, GWLP_WNDPROC);
    if (currentWndProc != HookedWndProc) {
        if (currentWndProc == m_OriginalWndProc) {
             std::cout << "[InputHookManager] WndProc reset detected. Re-hooking..." << std::endl;
             SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
        } else {
            std::cout << "[InputHookManager] WndProc changed to unknown. Re-hooking..." << std::endl;
            WNDPROC prev = (WNDPROC)SetWindowLongPtr(targetWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
            m_OriginalWndProc = prev;
        }
    }
}

LRESULT CALLBACK InputHookManager::GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) 
        return CallNextHookEx(Instance().m_MessageHook, nCode, wParam, lParam);

    if (nCode == HC_ACTION && wParam == PM_REMOVE) {
        MSG* msg = (MSG*)lParam;

        // 1. Handle Toggle
        if (msg->message == WM_KEYDOWN && msg->wParam == VK_HOME) {
            OverlayManager::Instance().ToggleVisibility();
            std::cout << "[InputHookManager] Home key detected." << std::endl;
            
            // Re-check hooks
            if (Instance().m_TargetWindow) {
                Instance().EnsureHooks(Instance().m_TargetWindow);
            }
            
            // Consume Home key
            msg->message = WM_NULL;
            return 0; 
        }

        // 2. Aggressive Blocking if Overlay is Visible
        if (OverlayManager::Instance().IsVisible()) {
            // Feed ImGui manually ONLY for Keyboard messages that we are about to nuke
            // Mouse messages are handled in HookedWndProc
            if (msg->message >= WM_KEYFIRST && msg->message <= WM_KEYLAST) {
                 ImGui_ImplWin32_WndProcHandler(msg->hwnd, msg->message, msg->wParam, msg->lParam);
            }

            // Special handling for Keyboard: Manually generate WM_CHAR
            if (msg->message == WM_KEYDOWN) {
                BYTE keyboardState[256];
                if (GetKeyboardState(keyboardState)) {
                    WCHAR buffer[16] = {};
                    UINT scanCode = (msg->lParam >> 16) & 0xFF;
                    int count = ToUnicode((UINT)msg->wParam, scanCode, keyboardState, buffer, 16, 0);
                    if (count > 0) {
                        for (int i = 0; i < count; i++) {
                            ImGui_ImplWin32_WndProcHandler(msg->hwnd, WM_CHAR, (WPARAM)buffer[i], 0);
                        }
                    }
                }
            }

            // Block Input Messages (Keyboard Only)
            switch (msg->message) {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                case WM_DEADCHAR:
                case WM_SYSCHAR:
                case WM_SYSDEADCHAR:
                case WM_UNICHAR:
                case WM_IME_CHAR:
                case WM_IME_STARTCOMPOSITION:
                case WM_IME_ENDCOMPOSITION:
                case WM_IME_COMPOSITION:
                case WM_IME_SETCONTEXT:
                case WM_INPUT:
                    // Nuke the message
                    msg->message = WM_NULL;
                    msg->lParam = 0;
                    msg->wParam = 0;
                    break;
            }
        }
    }

    return CallNextHookEx(Instance().m_MessageHook, nCode, wParam, lParam);
}

LRESULT CALLBACK InputHookManager::HookedWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (OverlayManager::Instance().IsInitialized()) {
        if (OverlayManager::Instance().IsVisible()) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
            
            // Block input
            switch (msg) {
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                case WM_DEADCHAR:
                case WM_SYSCHAR:
                case WM_SYSDEADCHAR:
                case WM_UNICHAR:
                case WM_IME_CHAR:
                case WM_IME_STARTCOMPOSITION:
                case WM_IME_ENDCOMPOSITION:
                case WM_IME_COMPOSITION:
                case WM_IME_SETCONTEXT:
                case WM_INPUT:
                    return 0;

                // Mouse Blocking: Pass to DefWindowProc to handle focus/activation, but bypass Game WndProc
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
                case WM_MOUSEHWHEEL:
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                case WM_XBUTTONDBLCLK:
                case WM_MOUSEMOVE:
                    return DefWindowProc(hwnd, msg, wParam, lParam);

                case WM_SETCURSOR:
                    if (ImGui::GetMouseCursor() != ImGuiMouseCursor_None) {
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                        return TRUE;
                    }
                    return DefWindowProc(hwnd, msg, wParam, lParam);
            }
        }
    }

    return CallWindowProc(Instance().m_OriginalWndProc, hwnd, msg, wParam, lParam);
}
