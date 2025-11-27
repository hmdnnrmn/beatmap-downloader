#pragma once
#include "../OverlayTab.h"
#include "../../speedtest_manager.h"
#include "imgui.h"
#include <string>

class SpeedtestTab : public OverlayTab {
public:
    std::string GetName() const override { return "Speedtest"; }

    void Render() override {
        SpeedtestState state = SpeedtestManager::Instance().GetState();
        
        // Status Text
        if (state == SpeedtestState::Running) {
            ImGui::Text("Status: Running...");
        } else if (state == SpeedtestState::Finished) {
            ImGui::Text("Status: Finished");
        } else {
            ImGui::Text("Status: Idle");
        }

        // Buttons
        if (state == SpeedtestState::Running) {
            if (ImGui::Button("Cancel")) {
                SpeedtestManager::Instance().CancelTest();
            }
        } else {
            if (ImGui::Button("Start Speedtest")) {
                SpeedtestManager::Instance().StartTest();
            }
        }

        ImGui::Separator();

        // Results Table
        if (ImGui::BeginTable("SpeedtestResults", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Mirror");
            ImGui::TableSetupColumn("Time (s)");
            ImGui::TableSetupColumn("Speed");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();

            auto results = SpeedtestManager::Instance().GetResults();
            
            // Find best result for highlighting
            int bestIndex = -1;
            double maxSpeed = -1.0;
            if (state == SpeedtestState::Finished) {
                for (int i = 0; i < results.size(); ++i) {
                    if (results[i].success && results[i].speedMBps > maxSpeed) {
                        maxSpeed = results[i].speedMBps;
                        bestIndex = i;
                    }
                }
            }

            for (int i = 0; i < results.size(); ++i) {
                const auto& res = results[i];
                ImGui::TableNextRow();
                
                if (i == bestIndex) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(0, 100, 0, 100));
                }

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", res.providerName.c_str());

                ImGui::TableSetColumnIndex(1);
                if (res.timeTakenSeconds > 0)
                    ImGui::Text("%.2f", res.timeTakenSeconds);
                else
                    ImGui::Text("-");

                ImGui::TableSetColumnIndex(2);
                if (res.speedMBps > 0)
                    ImGui::Text("%s", SpeedtestManager::FormatSpeed(res.speedMBps).c_str());
                else
                    ImGui::Text("-");

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", res.status.c_str());
            }

            ImGui::EndTable();
        }
    }
};
