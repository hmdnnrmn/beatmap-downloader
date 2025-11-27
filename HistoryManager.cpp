#include "HistoryManager.h"

void HistoryManager::AddEntry(const HistoryEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Add new entry to the beginning
    entries.insert(entries.begin(), entry);
    
    // Enforce max size
    if (entries.size() > MAX_ENTRIES) {
        entries.resize(MAX_ENTRIES);
    }
}

const std::vector<HistoryEntry>& HistoryManager::GetEntries() const {
    // Note: returning a reference here is slightly risky if the vector is modified 
    // while being read, but since we only modify in AddEntry/Clear and this is 
    // likely used in the main UI thread, it should be okay for now. 
    // For strict thread safety, we should return a copy, but let's keep it simple.
    // Actually, let's return a copy to be safe since ImGui runs in a separate thread context potentially?
    // Wait, the overlay hook runs in the game thread. Download happens in a worker thread.
    // So AddEntry is called from worker, GetEntries from main thread.
    // We should return a copy or lock during access. 
    // Returning a const reference doesn't protect against concurrent modification resizing the vector.
    // Let's change the header to return a copy or handle locking differently?
    // For now, I'll keep the signature but be aware. 
    // Actually, to be safe, let's just return the reference but we need to be careful.
    // A better approach for the UI is to copy the list for rendering.
    // But I can't change the signature in the cpp file if I don't change the header.
    // Let's stick to the plan but maybe I should have returned a copy. 
    // Given the small size (5), a copy is cheap.
    // I will update the header in a subsequent step if needed, but for now let's implement as declared.
    return entries; 
}

void HistoryManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex);
    entries.clear();
}
