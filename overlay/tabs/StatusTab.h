#pragma once
#include "OverlayTab.h"
#include "features/download_manager.h"
#include "imgui.h"
#include <string>

class StatusTab : public OverlayTab {
public:
    std::string GetName() const override { return "Status"; }

    void Render() override {
        DownloadState state = GetDownloadState();
        if (state.isDownloading) {
            std::string filenameStr(state.filename.begin(), state.filename.end());
            ImGui::Text("Downloading: %s", filenameStr.c_str());
            ImGui::ProgressBar(state.progress / 100.0f, ImVec2(-1, 0), 
                std::to_string((int)state.progress).c_str());
            ImGui::Text("%.2f MB / %.2f MB", 
                state.downloadedBytes / (1024.0f * 1024.0f), 
                state.totalBytes / (1024.0f * 1024.0f));
        } else {
            ImGui::Text("Idle. Waiting for beatmap link...");
        }
    }
};
