#pragma once
#include "OverlayTab.h"
#include "config/config_manager.h"
#include "providers/ProviderRegistry.h"
#include "imgui.h"
#include <string>
#include <vector>

class SettingsTab : public OverlayTab {
public:
    std::string GetName() const override { return "Settings"; }

    void Render() override {
        static char songsPath[256] = "";
        static int mirrorIndex = 0;
        static int metadataMirrorIndex = 0;
        static bool autoOpen = false;
        static bool clipboardEnabled = true;
        static bool initSettings = false;

        if (!initSettings) {
            std::wstring wPath = ConfigManager::Instance().GetSongsPath();
            std::string sPath(wPath.begin(), wPath.end());
            strcpy_s(songsPath, sPath.c_str());
            
            mirrorIndex = ConfigManager::Instance().GetDownloadMirrorIndex();
            metadataMirrorIndex = ConfigManager::Instance().GetMetadataMirrorIndex();
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

        if (ImGui::Button((std::string("Mirror: ") + providerNames[mirrorIndex]).c_str())) {
            mirrorIndex = (mirrorIndex + 1) % providerNames.size();
            ConfigManager::Instance().SetDownloadMirrorIndex(mirrorIndex);
        }

        // Filter for Metadata Mirrors (osu.direct and Catboy)
        std::vector<int> allowedMetadataIndices;
        for (size_t i = 0; i < providerNames.size(); ++i) {
            if (providerNames[i] == "osu.direct" || providerNames[i] == "Catboy") {
                allowedMetadataIndices.push_back((int)i);
            }
        }

        // Ensure current metadata index is valid
        bool isMetadataValid = false;
        for (int idx : allowedMetadataIndices) {
            if (metadataMirrorIndex == idx) {
                isMetadataValid = true;
                break;
            }
        }
        if (!isMetadataValid && !allowedMetadataIndices.empty()) {
            metadataMirrorIndex = allowedMetadataIndices[0];
            ConfigManager::Instance().SetMetadataMirrorIndex(metadataMirrorIndex);
        }

        std::string currentMetadataName = (metadataMirrorIndex >= 0 && metadataMirrorIndex < providerNames.size()) ? providerNames[metadataMirrorIndex] : "Unknown";
        if (ImGui::Button((std::string("Metadata: ") + currentMetadataName).c_str())) {
            if (!allowedMetadataIndices.empty()) {
                int currentPos = -1;
                for (size_t i = 0; i < allowedMetadataIndices.size(); ++i) {
                    if (allowedMetadataIndices[i] == metadataMirrorIndex) {
                        currentPos = (int)i;
                        break;
                    }
                }
                int nextPos = (currentPos + 1) % allowedMetadataIndices.size();
                metadataMirrorIndex = allowedMetadataIndices[nextPos];
                ConfigManager::Instance().SetMetadataMirrorIndex(metadataMirrorIndex);
            }
        }

        if (ImGui::Checkbox("Auto Open (.osz)", &autoOpen)) {
            ConfigManager::Instance().SetAutoOpen(autoOpen);
        }

        if (ImGui::Checkbox("Enable Clipboard Listener", &clipboardEnabled)) {
            ConfigManager::Instance().SetClipboardEnabled(clipboardEnabled);
        }
    }
};
