#pragma once
#include <string>

class Provider {
public:
    virtual ~Provider() = default;
    virtual std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) = 0;
    virtual std::string GetDownloadUrl(const std::wstring& id, bool isBeatmapId) = 0;
    virtual std::string GetName() const = 0;
};
