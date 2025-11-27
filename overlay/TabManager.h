#pragma once
#include <vector>
#include <memory>
#include <string>
#include "OverlayTab.h"
#include "imgui.h"

class TabManager {
public:
    static TabManager& Instance() {
        static TabManager instance;
        return instance;
    }

    void RegisterTab(std::shared_ptr<OverlayTab> tab) {
        m_Tabs.push_back(tab);
    }

    void Render() {
        if (m_Tabs.empty()) {
             // Log error or warning if possible, but for now let's just return
             return;
        }
        
        if (ImGui::BeginTabBar("MainTabs")) {
            for (auto& tab : m_Tabs) {
                if (ImGui::BeginTabItem(tab->GetName().c_str())) {
                    tab->Render();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
    }

    void ClearTabs() {
        m_Tabs.clear();
    }

private:
    std::vector<std::shared_ptr<OverlayTab>> m_Tabs;
};
