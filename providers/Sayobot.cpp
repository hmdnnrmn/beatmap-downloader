#include "Sayobot.h"
#include "Resolver.h"
#include "ProviderRegistry.h"

// Auto-register
__declspec(dllexport) bool g_RegisterSayobot = ProviderRegistry::Instance().Register("Sayobot", []() {
    return std::make_unique<SayobotProvider>();
});

std::wstring SayobotProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string SayobotProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://txy1.sayobot.cn/beatmaps/download/full/" + idStr;
}
