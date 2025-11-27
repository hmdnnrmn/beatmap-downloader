#include "notification_manager.h"
#include "utils/logging.h"

#define WM_TRAYICON (WM_USER + 1)

NotificationManager& NotificationManager::Instance() {
    static NotificationManager instance;
    return instance;
}

LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool NotificationManager::Initialize() {
    // Register a hidden window class for the tray icon
    WNDCLASS wc = {};
    wc.lpfnWndProc = NotificationWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"OsuDownloaderTray";
    RegisterClass(&wc);

    m_hHiddenWnd = CreateWindow(L"OsuDownloaderTray", L"", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

    if (!m_hHiddenWnd) {
        LogError("Failed to create notification window");
        return false;
    }

    // Initialize NOTIFYICONDATA
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(m_nid);
    m_nid.hWnd = m_hHiddenWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Use default app icon
    wcscpy_s(m_nid.szTip, L"osu! Beatmap Downloader");

    Shell_NotifyIcon(NIM_ADD, &m_nid);
    return true;
}

void NotificationManager::Cleanup() {
    Shell_NotifyIcon(NIM_DELETE, &m_nid);
    if (m_hHiddenWnd) {
        DestroyWindow(m_hHiddenWnd);
        m_hHiddenWnd = NULL;
    }
}

void NotificationManager::ShowNotification(const std::wstring& title, const std::wstring& message) {
    if (!m_hHiddenWnd) return;

    wcsncpy_s(m_nid.szInfoTitle, title.c_str(), ARRAYSIZE(m_nid.szInfoTitle));
    wcsncpy_s(m_nid.szInfo, message.c_str(), ARRAYSIZE(m_nid.szInfo));
    m_nid.dwInfoFlags = NIIF_INFO;
    m_nid.uTimeout = 3000; // 3 seconds

    Shell_NotifyIcon(NIM_MODIFY, &m_nid);
}
