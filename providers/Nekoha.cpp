#include "Nekoha.h"
#include "Resolver.h"
#include "ProviderRegistry.h"
#include "../network/HttpRequest.h"
#include <nlohmann/json.hpp>
#include <iostream>

// Auto-register
__declspec(dllexport) bool g_RegisterNekoha = ProviderRegistry::Instance().Register("Nekoha", []() {
    return std::make_unique<NekohaProvider>();
});

std::wstring NekohaProvider::ResolveBeatmapSetId(const std::wstring& id, bool isBeatmapId) {
    if (isBeatmapId) {
        return Resolver::ResolveSetIdFromBeatmapId(id);
    }
    return id;
}

std::string NekohaProvider::GetDownloadUrl(const std::wstring& id, bool isBeatmapId) {
    std::string idStr(id.begin(), id.end());
    return "https://mirror.nekoha.moe/api4/download/" + idStr;
}


