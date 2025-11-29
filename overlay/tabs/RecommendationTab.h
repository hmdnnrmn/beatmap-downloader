#pragma once
#include "OverlayTab.h"
#include "providers/OsuDirect.h"
#include "features/download_manager.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <thread>
#include <future>

class RecommendationTab : public OverlayTab {
public:
    std::string GetName() const override { return "Recommendation"; }

    void Render() override {
        static float minStars = 4.0f;
        static float maxStars = 5.0f;
        static std::vector<OsuDirectProvider::Recommendation> results;
        static std::mutex resultsMutex;
        static bool isSearching = false;
        static std::string statusMessage;

        // Filters
        static int currentStatusIdx = 1; // Default to Ranked
        const char* statusLabels[] = { "Any", "Ranked", "Loved", "Qualified", "Pending", "WIP", "Graveyard" };
        const int statusValues[] = { -999, 1, 4, 3, 0, -1, -2 };

        static int currentModeIdx = 1; // Default to Standard
        const char* modeLabels[] = { "Any", "Standard", "Taiko", "Catch", "Mania" };
        const int modeValues[] = { -999, 0, 1, 2, 3 };

        ImGui::Text("Get Beatmap Recommendations");
        
        // Filter Buttons
        if (ImGui::Button((std::string("Status: ") + statusLabels[currentStatusIdx]).c_str())) {
            currentStatusIdx = (currentStatusIdx + 1) % IM_ARRAYSIZE(statusLabels);
        }
        ImGui::SameLine();

        if (ImGui::Button((std::string("Mode: ") + modeLabels[currentModeIdx]).c_str())) {
            currentModeIdx = (currentModeIdx + 1) % IM_ARRAYSIZE(modeLabels);
        }
        
        ImGui::InputFloat("Min Stars", &minStars, 0.1f, 1.0f, "%.2f");
        ImGui::InputFloat("Max Stars", &maxStars, 0.1f, 1.0f, "%.2f");

        if (ImGui::Button("Get Recommendations")) {
            isSearching = true;
            statusMessage = "Fetching recommendations...";
            {
                std::lock_guard<std::mutex> lock(resultsMutex);
                results.clear();
            }

            int mode = modeValues[currentModeIdx];
            int status = statusValues[currentStatusIdx];

            std::thread([=]() {
                OsuDirectProvider provider;
                auto res = provider.GetRecommendations(minStars, maxStars, mode, status);
                
                {
                    std::lock_guard<std::mutex> lock(resultsMutex);
                    results = res;
                }
                
                statusMessage = res.empty() ? "No recommendations found." : "Found " + std::to_string(res.size()) + " maps. Fetching details...";
                isSearching = false;

                // Async fetch metadata
                for (size_t i = 0; i < res.size(); ++i) {
                    if (res[i].parentSetId > 0) {
                        auto setInfo = provider.GetBeatmapSetInfo(std::to_wstring(res[i].parentSetId));
                        if (setInfo.has_value()) {
                            std::lock_guard<std::mutex> lock(resultsMutex);
                            // Verify index is still valid and ID matches (in case of new search)
                            if (i < results.size() && results[i].parentSetId == res[i].parentSetId) {
                                results[i].artist = std::string(setInfo->artist.begin(), setInfo->artist.end());
                                results[i].title = std::string(setInfo->title.begin(), setInfo->title.end());
                            }
                        }
                    }
                }
                statusMessage = "Ready.";
            }).detach();
        }

        if (isSearching) {
            ImGui::TextDisabled("%s", statusMessage.c_str());
        } else {
            ImGui::Text("%s", statusMessage.c_str());
        }

        ImGui::Separator();
        ImGui::Spacing();

        // Results Table in Child Window
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        if (ImGui::BeginChild("RecommendationResultsChild", ImVec2(0, 0), true, ImGuiWindowFlags_None)) {
            if (ImGui::BeginTable("RecommendationResults", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Diff Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Stars", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableHeadersRow();

                std::lock_guard<std::mutex> lock(resultsMutex);
                int i = 0;
                for (const auto& map : results) {
                    ImGui::PushID(i++);
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    std::string name = map.artist + " - " + map.title;
                    ImGui::Text("%s", name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%s", map.diffName.c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.2f", map.stars);

                    ImGui::TableSetColumnIndex(3);
                    
                    std::wstring setId = std::to_wstring(map.parentSetId);
                    bool isDownloaded = HistoryManager::Instance().IsMapDownloaded(setId);

                    if (isDownloaded) {
                        ImGui::BeginDisabled();
                        ImGui::Button("Downloaded", ImVec2(-1, 0));
                        ImGui::EndDisabled();
                    } else {
                        if (ImGui::Button("Download", ImVec2(-1, 0))) {
                            // Use ParentSetID for download
                            std::wstring artistW(map.artist.begin(), map.artist.end());
                            std::wstring titleW(map.title.begin(), map.title.end());
                            std::thread([setId, artistW, titleW]() {
                                DownloadBeatmap(setId, false, artistW, titleW); 
                            }).detach(); 
                        }
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }
};
