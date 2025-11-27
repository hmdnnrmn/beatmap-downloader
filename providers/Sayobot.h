#pragma once
#include "Provider.h"

class SayobotProvider : public Provider {
public:
    std::wstring ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) override;
    std::string GetDownloadUrl(const std::wstring& id, bool isBeatmapId) override;
    std::string GetName() const override { return "Sayobot"; }
};
