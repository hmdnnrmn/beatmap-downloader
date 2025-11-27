#include "OsuDirect.h"
#include "Resolver.h"
#include "ProviderRegistry.h"

// Auto-register
__declspec(dllexport) bool g_RegisterOsuDirect = ProviderRegistry::Instance().Register("osu.direct", []() {
    return std::make_unique<OsuDirectProvider>();
});

std::wstring OsuDirectProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string OsuDirectProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://osu.direct/api/d/" + idStr;
}
