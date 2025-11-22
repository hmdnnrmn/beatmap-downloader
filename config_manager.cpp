#include "config_manager.h"
#include "logging.h"
#include <shlobj.h>
#include <iostream>
#include <filesystem>

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : m_autoOpen(true) {
    // Set config path to be next to the DLL
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandle(NULL), dllPath, MAX_PATH);
    std::wstring path(dllPath);
    m_configPath = path.substr(0, path.find_last_of(L"\\/")) + L"\\config.ini";
    
    m_downloadMirror = "https://catboy.best/d/";
    m_songsPath = GetDefaultSongsPath();
}

std::wstring ConfigManager::GetDefaultSongsPath() {
    wchar_t localAppData[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        std::wstring osuPath = std::wstring(localAppData) + L"\\osu!\\Songs";
        if (GetFileAttributesW(osuPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return osuPath;
        }
    }
    
    // Fallback to default installation path
    wchar_t* username = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&username, &len, L"USERNAME") == 0 && username != nullptr) {
        std::wstring path = L"C:\\Users\\" + std::wstring(username) + L"\\AppData\\Local\\osu!\\Songs";
        free(username);
        return path;
    }
    return L"";
}

bool ConfigManager::LoadConfig() {
    LogInfo("Loading config from: " + std::string(m_configPath.begin(), m_configPath.end()));

    // Check if config exists, if not create default
    if (GetFileAttributesW(m_configPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SaveConfig();
        return true;
    }

    wchar_t buffer[MAX_PATH];
    
    // Load Songs Path
    GetPrivateProfileStringW(L"General", L"SongsPath", m_songsPath.c_str(), buffer, MAX_PATH, m_configPath.c_str());
    m_songsPath = buffer;

    // Load Mirror
    GetPrivateProfileStringW(L"General", L"Mirror", L"https://catboy.best/d/", buffer, MAX_PATH, m_configPath.c_str());
    std::wstring mirrorW(buffer);
    m_downloadMirror = std::string(mirrorW.begin(), mirrorW.end());

    // Load AutoOpen
    m_autoOpen = GetPrivateProfileIntW(L"General", L"AutoOpen", 1, m_configPath.c_str()) != 0;

    LogInfo("Config loaded. Songs Path: " + std::string(m_songsPath.begin(), m_songsPath.end()));
    return true;
}

void ConfigManager::SaveConfig() {
    WritePrivateProfileStringW(L"General", L"SongsPath", m_songsPath.c_str(), m_configPath.c_str());
    
    std::wstring mirrorW(m_downloadMirror.begin(), m_downloadMirror.end());
    WritePrivateProfileStringW(L"General", L"Mirror", mirrorW.c_str(), m_configPath.c_str());
    
    WritePrivateProfileStringW(L"General", L"AutoOpen", m_autoOpen ? L"1" : L"0", m_configPath.c_str());
    
    LogInfo("Config saved");
}

std::wstring ConfigManager::GetSongsPath() const {
    return m_songsPath;
}

std::string ConfigManager::GetDownloadMirror() const {
    return m_downloadMirror;
}

bool ConfigManager::GetAutoOpen() const {
    return m_autoOpen;
}

void ConfigManager::SetSongsPath(const std::wstring& path) {
    m_songsPath = path;
    SaveConfig();
}

void ConfigManager::SetDownloadMirror(const std::string& mirror) {
    m_downloadMirror = mirror;
    SaveConfig();
}

void ConfigManager::SetAutoOpen(bool autoOpen) {
    m_autoOpen = autoOpen;
    SaveConfig();
}
