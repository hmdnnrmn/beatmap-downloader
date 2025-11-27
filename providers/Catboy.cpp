#include "Catboy.h"
#include "Resolver.h"
#include "ProviderRegistry.h"

// Auto-register
__declspec(dllexport) bool g_RegisterCatboy = ProviderRegistry::Instance().Register("Catboy", []() {
    return std::make_unique<CatboyProvider>();
});

std::wstring CatboyProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string CatboyProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://catboy.best/d/" + idStr;
}
