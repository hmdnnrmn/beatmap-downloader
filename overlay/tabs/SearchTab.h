
#pragma once
#include "OverlayTab.h"
#include "providers/ProviderRegistry.h"
#include "features/download_manager.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <future>

// Include for HistoryManager
#include "features/HistoryManager.h"

class SearchTab : public OverlayTab {
public:
    std::string GetName() const override { return "Search"; }

    void Render() override {
        static char queryBuf[256] = "";
        static int currentProviderIdx = 0;
        static std::vector<BeatmapSetInfo> results;
        static bool isSearching = false;
        static std::string statusMessage;

        const char* providers[] = { "osu.direct", "Nerinyan", "Catboy" };

        ImGui::Text("Search Beatmaps");
        
        // Provider Selection
        if (ImGui::Button((std::string("Mirror: ") + providers[currentProviderIdx]).c_str())) {
            currentProviderIdx = (currentProviderIdx + 1) % IM_ARRAYSIZE(providers);
        }
        ImGui::SameLine();

        // Filters
        static int currentStatusIdx = 0; // 0=Any, 1=Ranked, 2=Loved, 3=Qualified, 4=Pending, 5=WIP, 6=Graveyard
        const char* statusLabels[] = { "Any", "Ranked", "Loved", "Qualified", "Pending", "WIP", "Graveyard" };
        const int statusValues[] = { -999, 1, 4, 3, 0, -1, -2 }; // -999 = Any

        static int currentModeIdx = 0; // 0=Any, 1=Osu, 2=Taiko, 3=CtB, 4=Mania
        const char* modeLabels[] = { "Any", "Standard", "Taiko", "Catch", "Mania" };
        const int modeValues[] = { -999, 0, 1, 2, 3 }; // -999 = Any

        if (ImGui::Button((std::string("Status: ") + statusLabels[currentStatusIdx]).c_str())) {
            currentStatusIdx = (currentStatusIdx + 1) % IM_ARRAYSIZE(statusLabels);
        }
        ImGui::SameLine();

        if (ImGui::Button((std::string("Mode: ") + modeLabels[currentModeIdx]).c_str())) {
            currentModeIdx = (currentModeIdx + 1) % IM_ARRAYSIZE(modeLabels);
        }

        // Search Bar
        ImGui::InputText("Query", queryBuf, IM_ARRAYSIZE(queryBuf));
        ImGui::SameLine();
        
        if (ImGui::Button("Search") || (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
            if (strlen(queryBuf) > 0) {
                isSearching = true;
                statusMessage = "Searching...";
                results.clear();
                
                std::string query = queryBuf;
                std::string providerName = providers[currentProviderIdx];
                
                SearchFilter filter;
                if (statusValues[currentStatusIdx] != -999) filter.status = statusValues[currentStatusIdx];
                if (modeValues[currentModeIdx] != -999) filter.mode = modeValues[currentModeIdx];

                // Run search in a separate thread to avoid freezing UI
                std::thread([query, providerName, filter]() {
                    auto provider = ProviderRegistry::Instance().CreateProvider(providerName);
                    if (provider) {
                        auto res = provider->Search(query, filter);
                        // Simple thread safety for demo (should use proper synchronization in prod)
                        results = res;
                        statusMessage = res.empty() ? "No results found." : "Found " + std::to_string(res.size()) + " maps.";
                    } else {
                        statusMessage = "Provider not found.";
                    }
                    isSearching = false;
                }).detach();
            }
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
        if (ImGui::BeginChild("SearchResultsChild", ImVec2(0, 0), true, ImGuiWindowFlags_None)) {
            if (ImGui::BeginTable("SearchResults", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
                ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Artist", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Mapper", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableHeadersRow();

                int i = 0;
                for (const auto& map : results) {
                    ImGui::PushID(i++);
                    ImGui::TableNextRow();
                    
                    ImGui::TableSetColumnIndex(0);
                    std::string title(map.title.begin(), map.title.end());
                    ImGui::Text("%s", title.c_str());

                    ImGui::TableSetColumnIndex(1);
                    std::string artist(map.artist.begin(), map.artist.end());
                    ImGui::Text("%s", artist.c_str());

                    ImGui::TableSetColumnIndex(2);
                    std::string creator(map.creator.begin(), map.creator.end());
                    ImGui::Text("%s", creator.c_str());

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%s", map.status.c_str());

                    ImGui::TableSetColumnIndex(4);
                    std::wstring mapId = std::wstring(map.id.begin(), map.id.end());
                    bool isDownloaded = HistoryManager::Instance().IsMapDownloaded(mapId);

                    if (isDownloaded) {
                        ImGui::BeginDisabled();
                        ImGui::Button("Downloaded", ImVec2(-1, 0));
                        ImGui::EndDisabled();
                    } else {
                        if (ImGui::Button("Download", ImVec2(-1, 0))) {
                            std::wstring artist = map.artist;
                            std::wstring title = map.title;
                            std::thread([mapId, artist, title]() {
                                DownloadBeatmap(mapId, false, artist, title);
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
