#include "HistoryManager.h"

void HistoryManager::AddEntry(const HistoryEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Add new entry to the beginning
    entries.insert(entries.begin(), entry);
}

const std::vector<HistoryEntry>& HistoryManager::GetEntries() const {
    return entries; 
}

void HistoryManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    entries.clear();
}

bool HistoryManager::IsMapDownloaded(const std::wstring& id) {
    std::lock_guard<std::mutex> lock(mutex);
    for (const auto& entry : entries) {
        if (entry.id == id && (entry.status == "Success" || entry.status == "Skipped (Exists)")) {
            return true;
        }
    }
    return false;
}
