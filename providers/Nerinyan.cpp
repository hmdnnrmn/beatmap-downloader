#include "Nerinyan.h"
#include "Resolver.h"
#include "ProviderRegistry.h"

// Auto-register
__declspec(dllexport) bool g_RegisterNerinyan = ProviderRegistry::Instance().Register("Nerinyan", []() {
    return std::make_unique<NerinyanProvider>();
});

std::wstring NerinyanProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string NerinyanProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://api.nerinyan.moe/d/" + idStr;
}
