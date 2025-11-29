#include "config_manager.h"
#include "utils/logging.h"
#include <iostream>
#include <filesystem>

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : m_autoOpen(true), m_mirrorIndex(0), m_metadataMirrorIndex(0), m_clipboardEnabled(true) {
    // Set config path to be next to the DLL
    wchar_t dllPath[MAX_PATH];
    GetModuleFileNameW(GetModuleHandle(NULL), dllPath, MAX_PATH);
    std::wstring path(dllPath);
    m_configPath = path.substr(0, path.find_last_of(L"\\/")) + L"\\config.ini";
    
}



bool ConfigManager::LoadConfig() {
    LogInfo("Loading config from: " + std::string(m_configPath.begin(), m_configPath.end()));

    // Check if config exists, if not create default
    if (GetFileAttributesW(m_configPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        SaveConfig();
        return true;
    }

    wchar_t buffer[MAX_PATH];
    
    // Load Mirror Index
    m_mirrorIndex = GetPrivateProfileIntW(L"General", L"MirrorIndex", 0, m_configPath.c_str());
    m_metadataMirrorIndex = GetPrivateProfileIntW(L"General", L"MetadataMirrorIndex", 0, m_configPath.c_str());

    // Load AutoOpen
    m_autoOpen = GetPrivateProfileIntW(L"General", L"AutoOpen", 1, m_configPath.c_str()) != 0;

    // Load Clipboard Enabled
    m_clipboardEnabled = GetPrivateProfileIntW(L"General", L"ClipboardEnabled", 1, m_configPath.c_str()) != 0;

    LogInfo("Config loaded.");
    return true;
}

void ConfigManager::SaveConfig() {
    WritePrivateProfileStringW(L"General", L"MirrorIndex", std::to_wstring(m_mirrorIndex).c_str(), m_configPath.c_str());
    WritePrivateProfileStringW(L"General", L"MetadataMirrorIndex", std::to_wstring(m_metadataMirrorIndex).c_str(), m_configPath.c_str());
    
    WritePrivateProfileStringW(L"General", L"AutoOpen", m_autoOpen ? L"1" : L"0", m_configPath.c_str());

    WritePrivateProfileStringW(L"General", L"ClipboardEnabled", m_clipboardEnabled ? L"1" : L"0", m_configPath.c_str());
    
    LogInfo("Config saved");
}



int ConfigManager::GetDownloadMirrorIndex() const {
    return m_mirrorIndex;
}

int ConfigManager::GetMetadataMirrorIndex() const {
    return m_metadataMirrorIndex;
}

bool ConfigManager::GetAutoOpen() const {
    return m_autoOpen;
}

bool ConfigManager::IsClipboardEnabled() const {
    return m_clipboardEnabled;
}



void ConfigManager::SetDownloadMirrorIndex(int index) {
    m_mirrorIndex = index;
    SaveConfig();
}

void ConfigManager::SetMetadataMirrorIndex(int index) {
    m_metadataMirrorIndex = index;
    SaveConfig();
}

void ConfigManager::SetAutoOpen(bool autoOpen) {
    m_autoOpen = autoOpen;
    SaveConfig();
}

void ConfigManager::SetClipboardEnabled(bool enabled) {
    m_clipboardEnabled = enabled;
    SaveConfig();
}
