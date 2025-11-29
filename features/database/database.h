#pragma once
#include <string>
#include <vector>
#include <set>
#include "database_structure.h"

class OsuDatabase {
public:
    OsuDatabase();
    ~OsuDatabase();

    bool Load(const std::string& path);

    const OsuDbInfo& GetDbInfo() const { return dbInfo; }
    const std::set<int>& GetBeatmapIds() const { return existingBeatmapIds; }
    const std::set<int>& GetSetIds() const { return existingSetIds; }

private:
    OsuDbInfo dbInfo;
    std::set<int> existingBeatmapIds;
    std::set<int> existingSetIds;
};
