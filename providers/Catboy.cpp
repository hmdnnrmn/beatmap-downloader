#include "Catboy.h"
#include "Resolver.h"
#include "ProviderRegistry.h"
#include "../network/HttpRequest.h"
#include <nlohmann/json.hpp>
#include <iostream>

// Auto-register
__declspec(dllexport) bool g_RegisterCatboy = ProviderRegistry::Instance().Register("Catboy", []() {
    return std::make_unique<CatboyProvider>();
});

std::optional<BeatmapSetInfo> CatboyProvider::GetBeatmapSetInfo(const std::wstring& setId) {
    std::string idStr(setId.begin(), setId.end());
    std::string url = "https://catboy.best/api/s/" + idStr;
    std::string response;

    if (network::HttpRequest::Get(url, response)) {
        try {
            auto json = nlohmann::json::parse(response);
            
            BeatmapSetInfo info;
            std::string title = json.value("Title", "Unknown");
            info.title = std::wstring(title.begin(), title.end());

            std::string artist = json.value("Artist", "Unknown");
            info.artist = std::wstring(artist.begin(), artist.end());

            std::string creator = json.value("Creator", "Unknown");
            info.creator = std::wstring(creator.begin(), creator.end());

            int id = json.value("SetID", 0);
            info.id = std::to_wstring(id);

            int status = json.value("RankedStatus", 0);
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
            return info;
        } catch (const std::exception& e) {
            std::cerr << "JSON Parse Error (Catboy GetBeatmapSetInfo): " << e.what() << std::endl;
        }
    }
    return std::nullopt;
}

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


#include <nlohmann/json.hpp>
#include <iostream>

std::vector<BeatmapSetInfo> CatboyProvider::Search(const std::string& query, const SearchFilter& filter) {
    std::string encodedQuery = network::HttpRequest::UrlEncode(query);
    std::string url = "https://catboy.best/api/search?q=" + encodedQuery + "&amount=20";

    if (filter.mode.has_value()) {
        url += "&mode=" + std::to_string(filter.mode.value());
    }

    if (filter.status.has_value()) {
        url += "&status=" + std::to_string(filter.status.value());
    }

    std::string response;
    
    if (network::HttpRequest::Get(url, response)) {
        try {
            auto json = nlohmann::json::parse(response);
            std::vector<BeatmapSetInfo> results;
            
            for (const auto& item : json) {
                BeatmapSetInfo info;
                
                std::string title = item.value("Title", "Unknown");
                info.title = std::wstring(title.begin(), title.end());
                
                std::string artist = item.value("Artist", "Unknown");
                info.artist = std::wstring(artist.begin(), artist.end());
                
                std::string creator = item.value("Creator", "Unknown");
                info.creator = std::wstring(creator.begin(), creator.end());
                
                int id = item.value("SetID", 0);
                info.id = std::to_wstring(id);
                
                int status = item.value("RankedStatus", 0);
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
