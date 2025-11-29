#include "OsuDirect.h"
#include "Resolver.h"
#include "ProviderRegistry.h"
#include "../network/HttpRequest.h"
#include <nlohmann/json.hpp>
#include <iostream>

// Auto-register
__declspec(dllexport) bool g_RegisterOsuDirect = ProviderRegistry::Instance().Register("osu.direct", []() {
    return std::make_unique<OsuDirectProvider>();
});

std::optional<BeatmapSetInfo> OsuDirectProvider::GetBeatmapSetInfo(const std::wstring& setId) {
    std::string idStr(setId.begin(), setId.end());
    std::string url = "https://osu.direct/api/s/" + idStr;
    std::string response;

    if (network::HttpRequest::Get(url, response)) {
        try {
            auto json = nlohmann::json::parse(response);
            // Check if it's an array (some APIs return array of 1) or object
            if (json.is_array() && !json.empty()) {
                json = json[0];
            }

            BeatmapSetInfo info;
            std::string title = json.value("Title", "Unknown");
            info.title = std::wstring(title.begin(), title.end());

            std::string artist = json.value("Artist", "Unknown");
            info.artist = std::wstring(artist.begin(), artist.end());

            std::string creator = json.value("Creator", "Unknown");
            info.creator = std::wstring(creator.begin(), creator.end());

            int id = json.value("id", 0);
            info.id = std::to_wstring(id);

            // "ranked" is the integer status in v2 API
            int status = json.value("ranked", 0);
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
            std::cerr << "JSON Parse Error (GetBeatmapSetInfo): " << e.what() << std::endl;
        }
    }
    return std::nullopt;
}

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

std::vector<BeatmapSetInfo> OsuDirectProvider::Search(const std::string& query, const SearchFilter& filter) {
    std::string encodedQuery = network::HttpRequest::UrlEncode(query);
    std::string url = "https://osu.direct/api/v2/search?q=" + encodedQuery + "&amount=20";
    
    if (filter.status.has_value()) {
        url += "&status=" + std::to_string(filter.status.value());
    }
    
    if (filter.mode.has_value()) {
        url += "&mode=" + std::to_string(filter.mode.value());
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
                
                // "ranked" is the integer status in v2 API
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

std::vector<OsuDirectProvider::Recommendation> OsuDirectProvider::GetRecommendations(float minStars, float maxStars, int mode, int status) {
    // https://osu.direct/api/recommend?amount=20&minStars=4.6&maxStars=5.1&mode=0&status=1
    std::string url = "https://osu.direct/api/recommend?amount=10";
    url += "&minStars=" + std::to_string(minStars);
    url += "&maxStars=" + std::to_string(maxStars);
    
    if (mode >= 0) {
        url += "&mode=" + std::to_string(mode);
    }
    
    if (status >= -2) {
        url += "&status=" + std::to_string(status);
    }

    std::string response;
    if (network::HttpRequest::Get(url, response)) {
        try {
            auto json = nlohmann::json::parse(response);
            std::vector<Recommendation> results;

            for (const auto& item : json) {
                Recommendation rec;
                rec.parentSetId = item.value("ParentSetID", 0);
                rec.beatmapId = item.value("BeatmapID", 0);
                rec.diffName = item.value("DiffName", "Unknown");
                rec.stars = item.value("DifficultyRating", 0.0f);
                
                // Metadata will be fetched asynchronously by the UI
                rec.artist = "Loading...";
                rec.title = "Loading...";
                
                /*
                // Fetch metadata
                if (rec.parentSetId > 0) {
                    auto setInfo = GetBeatmapSetInfo(std::to_wstring(rec.parentSetId));
                    if (setInfo.has_value()) {
                        rec.artist = std::string(setInfo->artist.begin(), setInfo->artist.end());
                        rec.title = std::string(setInfo->title.begin(), setInfo->title.end());
                    } else {
                        rec.artist = "Unknown";
                        rec.title = "Unknown";
                    }
                }
                */

                results.push_back(rec);
            }
            return results;
        }
        catch (const std::exception& e) {
            std::cerr << "JSON Parse Error (GetRecommendations): " << e.what() << std::endl;
        }
    }
    return {};
}
