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

#include "../network/HttpRequest.h"
#include <nlohmann/json.hpp>
#include <iostream>

std::vector<BeatmapSetInfo> NerinyanProvider::Search(const std::string& query, const SearchFilter& filter) {
    std::string encodedQuery = network::HttpRequest::UrlEncode(query);
    std::string url = "https://api.nerinyan.moe/search?q=" + encodedQuery + "&ps=20";

    if (filter.mode.has_value()) {
        url += "&m=" + std::to_string(filter.mode.value());
    }

    if (filter.status.has_value()) {
        url += "&s=" + std::to_string(filter.status.value());
    }

    std::string response;
    
    if (network::HttpRequest::Get(url, response)) {
        try {
            auto json = nlohmann::json::parse(response);
            std::vector<BeatmapSetInfo> results;
            
            for (const auto& item : json) {
                BeatmapSetInfo info;
                
                std::string title = item.value("title", "Unknown");
                info.title = std::wstring(title.begin(), title.end());
                
                std::string artist = item.value("artist", "Unknown");
                info.artist = std::wstring(artist.begin(), artist.end());
                
                std::string creator = item.value("creator", "Unknown");
                info.creator = std::wstring(creator.begin(), creator.end());
                
                int id = item.value("id", 0);
                info.id = std::to_wstring(id);
                
                int status = item.value("ranked", 0);
                switch (status) {
                    case -2: info.status = "Graveyard"; break;
                    case -1: info.status = "WIP"; break;
                    case 0: info.status = "Pending"; break;
                    case 1: info.status = "Ranked"; break;
                    case 2: info.status = "Approved"; break;
                    case 3: info.status = "Qualified"; break;
                    case 4: info.status = "Loved"; break;
                    default: info.status = "Unknown"; break;
                }

                results.push_back(info);
            }
            return results;
        } catch (const std::exception& e) {
            std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        }
    }
    return {};
}
