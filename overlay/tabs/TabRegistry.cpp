#include "TabRegistry.h"
#include "TabManager.h"
#include "StatusTab.h"
#include "SettingsTab.h"
#include "SpeedtestTab.h"
#include "HistoryTab.h"
#include "SearchTab.h"
#include "RecommendationTab.h"
#include <memory>

namespace TabRegistry {
    void RegisterAllTabs() {
        TabManager::Instance().ClearTabs();
        TabManager::Instance().RegisterTab(std::make_shared<StatusTab>());
        TabManager::Instance().RegisterTab(std::make_shared<HistoryTab>());
        TabManager::Instance().RegisterTab(std::make_shared<SearchTab>());
        TabManager::Instance().RegisterTab(std::make_shared<RecommendationTab>());
        TabManager::Instance().RegisterTab(std::make_shared<SettingsTab>());
        TabManager::Instance().RegisterTab(std::make_shared<SpeedtestTab>());
    }
}
