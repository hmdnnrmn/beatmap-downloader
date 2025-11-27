#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <functional>

struct SpeedtestResult {
    std::string providerName;
    double timeTakenSeconds;
    double speedMBps;
    bool success;
    std::string status; // e.g., "Done", "Failed", "Testing..."
};

enum class SpeedtestState {
    Idle,
    Running,
    Finished
};


class SpeedtestManager {
public:
    static SpeedtestManager& Instance();

    void StartTest();
    void CancelTest();
    void Update(); // Called every frame
    std::vector<SpeedtestResult> GetResults();
    SpeedtestState GetState() const;
    
    // Helper to format speed
    static std::string FormatSpeed(double speedMBps);

private:
    SpeedtestManager();
    ~SpeedtestManager();
    
    void RunNextTest();
    void FinishTest();

    SpeedtestState m_State = SpeedtestState::Idle;
    std::vector<SpeedtestResult> m_Results;
    std::mutex m_Mutex;
    
    // Test execution state
    int m_CurrentProviderIndex = 0;
    bool m_IsCancelled = false;
    std::vector<std::string> m_ProviderNames;
    
    // Constants
    const std::wstring TEST_BEATMAP_ID = L"760464"; // User requested map
};
