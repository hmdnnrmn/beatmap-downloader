#pragma once
#include <string>
#include <vector>
#include <ctime>
#include <mutex>

struct HistoryEntry {
    std::wstring title;
    std::wstring id;
    std::string status; // "Success", "Failed", etc.
    std::time_t timestamp;
};

class HistoryManager {
public:
    static HistoryManager& Instance() {
        static HistoryManager instance;
        return instance;
    }

    void AddEntry(const HistoryEntry& entry);
    const std::vector<HistoryEntry>& GetEntries() const;
    void Clear();

private:
    HistoryManager() = default;
    ~HistoryManager() = default;
    HistoryManager(const HistoryManager&) = delete;
    HistoryManager& operator=(const HistoryManager&) = delete;

    std::vector<HistoryEntry> entries;
    mutable std::mutex mutex;
    const size_t MAX_ENTRIES = 5;
};
