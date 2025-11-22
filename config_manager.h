#pragma once
#include <string>
#include <windows.h>

class ConfigManager {
public:
    static ConfigManager& Instance();

    bool LoadConfig();
    void SaveConfig();

    std::wstring GetSongsPath() const;
    std::string GetDownloadMirror() const;
    bool GetAutoOpen() const;

    void SetSongsPath(const std::wstring& path);
    void SetDownloadMirror(const std::string& mirror);
    void SetAutoOpen(bool autoOpen);

private:
    ConfigManager();
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    std::wstring m_configPath;
    std::wstring m_songsPath;
    std::string m_downloadMirror;
    bool m_autoOpen;

    std::wstring GetDefaultSongsPath();
};
