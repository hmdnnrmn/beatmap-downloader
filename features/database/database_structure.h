#pragma once
#include <string>

struct OsuDbInfo {
    int version;
    int folderCount;
    bool accountUnlocked;
    long long accountUnlockDate;
    std::string playerName;
    int beatmapCount;
};
