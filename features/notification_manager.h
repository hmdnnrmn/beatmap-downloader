#pragma once
#include <windows.h>
#include <string>

class NotificationManager {
public:
    static NotificationManager& Instance();

    bool Initialize();
    void Cleanup();
    void ShowNotification(const std::wstring& title, const std::wstring& message);

private:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NotificationManager(const NotificationManager&) = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;

    HWND m_hHiddenWnd = NULL;
    NOTIFYICONDATA m_nid = {};
};
