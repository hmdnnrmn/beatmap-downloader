#include "Beatconnect.h"
#include "Resolver.h"
#include "ProviderRegistry.h"

// Auto-register
__declspec(dllexport) bool g_RegisterBeatconnect = ProviderRegistry::Instance().Register("Beatconnect", []() {
    return std::make_unique<BeatconnectProvider>();
});

std::wstring BeatconnectProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string BeatconnectProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://beatconnect.io/b/" + idStr;
}
