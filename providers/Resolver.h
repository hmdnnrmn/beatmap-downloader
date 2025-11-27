#pragma once
#include <string>

class Resolver {
public:
    // Resolves a Beatmap ID (from /b/ link) to a Beatmap Set ID (from /beatmapsets/ link)
    // Returns empty string if resolution fails or if input is not a valid ID
    static std::wstring ResolveSetIdFromBeatmapId(const std::wstring& beatmapId);
};
