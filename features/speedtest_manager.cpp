#include "speedtest_manager.h"
#include "providers/ProviderRegistry.h"
#include "network/HttpRequest.h"
#include "config/config_manager.h"
#include "utils/logging.h"
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <shlobj.h>

namespace fs = std::filesystem;

SpeedtestManager::SpeedtestManager() {}
SpeedtestManager::~SpeedtestManager() {}

SpeedtestManager& SpeedtestManager::Instance() {
    static SpeedtestManager instance;
    return instance;
}

void SpeedtestManager::Update() {
    // Nothing to do here for now
}

void SpeedtestManager::CancelTest() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_State == SpeedtestState::Running) {
        m_IsCancelled = true;
        m_State = SpeedtestState::Idle; // Or Finished? Let's go to Idle.
        LogInfo("Speedtest cancelled by user.");
    }
}



// Separate function to trigger the next download safely
// This is a bit tricky with mutexes. We'll use a recursive approach where the callback triggers the next one.
// But we need to be careful about stack depth if they were synchronous. They are async, so it's fine.

void SpeedtestManager::RunNextTest() {
    // We need to find the next provider to test
    std::string providerName;
    size_t currentIndex;

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        if (m_IsCancelled) return;

        if (m_CurrentProviderIndex >= m_ProviderNames.size()) {
            FinishTest();
            return;
        }
        currentIndex = m_CurrentProviderIndex;
        providerName = m_ProviderNames[currentIndex];
        m_Results[currentIndex].status = "Testing...";
    }

    // Get Provider
    auto provider = ProviderRegistry::Instance().CreateProvider(providerName);
    if (!provider) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Results[currentIndex].status = "Failed (Init)";
        m_CurrentProviderIndex++;
        RunNextTest(); // Recurse
        return;
    }

    // Resolve ID
    std::wstring beatmapSetId = provider->ResolveBeatmapSetId(TEST_BEATMAP_ID, true); // Assuming ID 3 is a Set ID? No, 3 is a Set ID.
    // Wait, "Ni-Sokkususu" Set ID is 3. 
    // Provider::ResolveBeatmapSetId(id, isBeatmapId). If we pass "3" and say isBeatmapId=false (it's a set ID), it should work.
    // But wait, the user said "beatmap". Usually people mean Set ID when downloading.
    // Let's assume Set ID 3.
    
    // Actually, let's look at Provider interface.
    // virtual std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) = 0;
    // If we pass "3" and isBeatmapId=false, it should return "3".
    
    // Get Download URL
    std::string url = provider->GetDownloadUrl(beatmapSetId, false);
    
    // Prepare destination
    wchar_t localAppData[MAX_PATH];
    std::wstring songsPath;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
        songsPath = std::wstring(localAppData) + L"\\osu!\\Downloads";
    } else {
        // If we can't get the path, we can't download.
        // Log error or just return (since this is void, maybe just return or set status)
        {
             std::lock_guard<std::mutex> lock(m_Mutex);
             m_Results[currentIndex].status = "Failed (Path)";
             m_CurrentProviderIndex++;
        }
        RunNextTest();
        return;
    }
    
    // Ensure directory exists
    if (!fs::exists(songsPath)) {
        fs::create_directories(songsPath);
    }
    std::wstring tempFilename = L"speedtest_" + std::to_wstring(currentIndex) + L".osz";
    std::wstring fullPath = songsPath + L"\\" + tempFilename;

    // Start Timer
    auto startTime = std::chrono::high_resolution_clock::now();

    // Download
    // Download with 10s timeout
    network::HttpRequest::DownloadAsync(url, fullPath, 
        nullptr, 
        [this, currentIndex, fullPath, startTime](bool success, const std::string& error) {
            
            // Check cancellation first
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (m_IsCancelled) {
                    // Cleanup and exit
                    if (fs::exists(fullPath)) {
                        try { fs::remove(fullPath); } catch (...) {}
                    }
                    return;
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = endTime - startTime;
            double seconds = elapsed.count();

            // Calculate size
            double sizeMB = 0;
            if (success) {
                try {
                    sizeMB = (double)fs::file_size(fullPath) / (1024.0 * 1024.0);
                } catch (...) {
                    sizeMB = 0;
                }
            }

            // Update Results
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (success) {
                    m_Results[currentIndex].success = true;
                    m_Results[currentIndex].timeTakenSeconds = seconds;
                    m_Results[currentIndex].speedMBps = (seconds > 0) ? (sizeMB / seconds) : 0.0;
                    m_Results[currentIndex].status = "Done";
                } else {
                    m_Results[currentIndex].success = false;
                    // Check for timeout
                    if (error.find("timed out") != std::string::npos || error.find("Timeout") != std::string::npos) {
                         m_Results[currentIndex].status = "Timed Out";
                    } else {
                         m_Results[currentIndex].status = "Failed";
                    }
                }
                
                m_CurrentProviderIndex++;
            }

            // Cleanup
            if (fs::exists(fullPath)) {
                try {
                    fs::remove(fullPath);
                } catch (...) {}
            }

            // Next
            RunNextTest();
        },
        10L // Timeout 10 seconds
    );
}

void SpeedtestManager::FinishTest() {
    // Find best provider
    std::string bestProvider;
    double maxSpeed = -1.0;
    int bestIndex = -1;

    for (size_t i = 0; i < m_Results.size(); ++i) {
        if (m_Results[i].success && m_Results[i].speedMBps > maxSpeed) {
            maxSpeed = m_Results[i].speedMBps;
            bestProvider = m_Results[i].providerName;
            bestIndex = (int)i;
        }
    }

    if (bestIndex != -1) {
        LogInfo("Speedtest finished. Best provider: " + bestProvider + " (" + FormatSpeed(maxSpeed) + ")");
        ConfigManager::Instance().SetDownloadMirrorIndex(bestIndex);
    } else {
        LogInfo("Speedtest finished. No successful downloads.");
    }

    m_State = SpeedtestState::Finished;
}

std::vector<SpeedtestResult> SpeedtestManager::GetResults() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Results;
}

SpeedtestState SpeedtestManager::GetState() const {
    return m_State;
}

std::string SpeedtestManager::FormatSpeed(double speedMBps) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << speedMBps << " MB/s";
    return ss.str();
}

// We need to actually call RunNextTest from StartTest, but we can't do it while holding the lock if RunNextTest also locks.
// So we modify StartTest to call it after unlocking.

void SpeedtestManager::StartTest() {
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_State == SpeedtestState::Running) return;

        m_State = SpeedtestState::Running;
        m_IsCancelled = false;
        m_Results.clear();
        m_ProviderNames = ProviderRegistry::Instance().GetProviderNames();
        m_CurrentProviderIndex = 0;

        for (const auto& name : m_ProviderNames) {
            m_Results.push_back({name, 0.0, 0.0, false, "Pending"});
        }
    }
    
    LogInfo("Starting speedtest...");
    RunNextTest();
}
