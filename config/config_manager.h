#pragma once
#include <string>
#include <windows.h>

class ConfigManager {
public:
    static ConfigManager& Instance();

    bool LoadConfig();
    void SaveConfig();

    std::wstring GetSongsPath() const;

    int GetDownloadMirrorIndex() const;
    int GetMetadataMirrorIndex() const;
    bool GetAutoOpen() const;
    bool IsClipboardEnabled() const;

    void SetSongsPath(const std::wstring& path);
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
    std::wstring m_songsPath;
    int m_mirrorIndex;
    int m_metadataMirrorIndex;
    bool m_autoOpen;
    bool m_clipboardEnabled;

    std::wstring GetDefaultSongsPath();
};
