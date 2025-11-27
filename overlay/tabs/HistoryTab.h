#pragma once
#include "../OverlayTab.h"
#include "../../HistoryManager.h"
#include "imgui.h"
#include <ctime>
#include <string>

class HistoryTab : public OverlayTab {
public:
    std::string GetName() const override { return "History"; }

    void Render() override {
        ImGui::Text("Recent Downloads (Max 5)");
        if (ImGui::Button("Clear History")) {
            HistoryManager::Instance().Clear();
        }

        ImGui::Spacing();

        if (ImGui::BeginTable("HistoryTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            const auto& entries = HistoryManager::Instance().GetEntries();
            for (const auto& entry : entries) {
                ImGui::TableNextRow();
                
                ImGui::TableSetColumnIndex(0);
                std::string titleStr(entry.title.begin(), entry.title.end());
                ImGui::Text("%s", titleStr.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", entry.status.c_str());

                ImGui::TableSetColumnIndex(2);
                char timeBuf[32];
                struct tm timeInfo;
                localtime_s(&timeInfo, &entry.timestamp);
                strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeInfo);
                ImGui::Text("%s", timeBuf);
            }
            ImGui::EndTable();
        }
    }
};
