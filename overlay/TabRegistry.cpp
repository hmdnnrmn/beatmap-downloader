#include "TabRegistry.h"
#include "TabManager.h"
#include "tabs/StatusTab.h"
#include "tabs/SettingsTab.h"
#include "tabs/SpeedtestTab.h"
#include "tabs/HistoryTab.h" // Added include
#include <memory>

namespace TabRegistry {
    void RegisterAllTabs() {
        TabManager::Instance().ClearTabs();
        TabManager::Instance().RegisterTab(std::make_shared<StatusTab>());
        TabManager::Instance().RegisterTab(std::make_shared<HistoryTab>()); // Registered HistoryTab
        TabManager::Instance().RegisterTab(std::make_shared<SettingsTab>());
        TabManager::Instance().RegisterTab(std::make_shared<SpeedtestTab>());
    }
}
