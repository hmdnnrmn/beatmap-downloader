#pragma once
#include <string>
#include <vector>

#include <optional>

struct SearchFilter {
    std::optional<int> status;
    std::optional<int> mode;
};

struct BeatmapSetInfo {
    std::wstring title;
    std::wstring artist;
    std::wstring creator;
    std::wstring id;
    std::string status;
};

class Provider {
public:
    virtual ~Provider() = default;
    virtual std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) = 0;
    virtual std::string GetDownloadUrl(const std::wstring& id, bool isBeatmapId) = 0;
    virtual std::string GetName() const = 0;
    virtual std::vector<BeatmapSetInfo> Search(const std::string& query, const SearchFilter& filter = {}) { return {}; }
    virtual std::optional<BeatmapSetInfo> GetBeatmapSetInfo(const std::wstring& setId) { return std::nullopt; }
};
