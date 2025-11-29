#pragma once
#include <string>
#include <windows.h>

class ConfigManager {
public:
    static ConfigManager& Instance();

    bool LoadConfig();
    void SaveConfig();

    int GetDownloadMirrorIndex() const;
    int GetMetadataMirrorIndex() const;
    bool GetAutoOpen() const;
    bool IsClipboardEnabled() const;

    void SetDownloadMirrorIndex(int index);
    void SetMetadataMirrorIndex(int index);
    void SetAutoOpen(bool autoOpen);
    void SetClipboardEnabled(bool enabled);

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::wstring m_configPath;

    int m_mirrorIndex;
    int m_metadataMirrorIndex;
    bool m_autoOpen;
    bool m_clipboardEnabled;


};
