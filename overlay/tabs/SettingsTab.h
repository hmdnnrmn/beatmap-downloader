#pragma once
#include "../OverlayTab.h"
#include "../../config_manager.h"
#include "../../providers/ProviderRegistry.h"
#include "imgui.h"
#include <string>
#include <vector>

class SettingsTab : public OverlayTab {
public:
    std::string GetName() const override { return "Settings"; }

    void Render() override {
        static char songsPath[256] = "";
        static int mirrorIndex = 0;
        static bool autoOpen = false;
        static bool clipboardEnabled = true;
        static bool initSettings = false;

        if (!initSettings) {
            std::wstring wPath = ConfigManager::Instance().GetSongsPath();
            std::string sPath(wPath.begin(), wPath.end());
            strcpy_s(songsPath, sPath.c_str());
            
            mirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
            autoOpen = ConfigManager::Instance().GetAutoOpen();
            clipboardEnabled = ConfigManager::Instance().IsClipboardEnabled();
            initSettings = true;
        }

        if (ImGui::InputText("Songs Path", songsPath, IM_ARRAYSIZE(songsPath))) {
            std::string sPath = songsPath;
            std::wstring wPath(sPath.begin(), sPath.end());
            ConfigManager::Instance().SetSongsPath(wPath);
        }
        
        std::vector<std::string> providerNames = ProviderRegistry::Instance().GetProviderNames();
        std::vector<const char*> mirrors;
        for (const auto& name : providerNames) {
            mirrors.push_back(name.c_str());
        }

        if (ImGui::Combo("Mirror", &mirrorIndex, mirrors.data(), (int)mirrors.size())) {
            ConfigManager::Instance().SetDownloadMirrorIndex(mirrorIndex);
        }

        if (ImGui::Checkbox("Auto Open (.osz)", &autoOpen)) {
            ConfigManager::Instance().SetAutoOpen(autoOpen);
        }

        if (ImGui::Checkbox("Enable Clipboard Listener", &clipboardEnabled)) {
            ConfigManager::Instance().SetClipboardEnabled(clipboardEnabled);
        }
    }
};
