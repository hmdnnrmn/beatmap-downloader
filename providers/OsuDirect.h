#pragma once
#include "Provider.h"

class OsuDirectProvider : public Provider {
public:
    struct Recommendation {
        int parentSetId;
        int beatmapId;
        std::string diffName;
        float stars;
        std::string artist;
        std::string title;
    };

    std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) override;
    std::string GetDownloadUrl(const std::wstring& id, bool isBeatmapId) override;
    std::string GetName() const override { return "osu.direct"; }
    std::vector<BeatmapSetInfo> Search(const std::string& query, const SearchFilter& filter = {}) override;
    std::optional<BeatmapSetInfo> GetBeatmapSetInfo(const std::wstring& setId) override;
    std::vector<Recommendation> GetRecommendations(float minStars, float maxStars, int mode = 0, int status = 1);
};
