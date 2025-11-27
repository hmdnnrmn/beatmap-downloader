#pragma once
#include "Provider.h"

class CatboyProvider : public Provider {
public:
    std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) override;
    std::string GetDownloadUrl(const std::wstring& id, bool isBeatmapId) override;
    std::string GetName() const override { return "Catboy"; }
    std::vector<BeatmapSetInfo> Search(const std::string& query, const SearchFilter& filter = {}) override;
    std::optional<BeatmapSetInfo> GetBeatmapSetInfo(const std::wstring& setId) override;
};
