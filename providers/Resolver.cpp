#include "Resolver.h"
#include "../network/HttpRequest.h"
#include "../logging.h"
#include <iostream>

std::wstring Resolver::ResolveSetIdFromBeatmapId(const std::wstring& beatmapId) {
    std::string idStr(beatmapId.begin(), beatmapId.end());
    std::string osuUrl = "https://osu.ppy.sh/b/" + idStr;
    std::string redirectUrl;
    std::string error;
    
    LogInfo("Resolving BeatmapSet ID from: " + osuUrl);
    
    if (network::HttpRequest::GetRedirectUrl(osuUrl, redirectUrl, &error)) {
        // Expected format: https://osu.ppy.sh/beatmapsets/547857#osu/1160293
        std::string searchKey = "/beatmapsets/";
        size_t pos = redirectUrl.find(searchKey);
        if (pos != std::string::npos) {
            size_t start = pos + searchKey.length();
            size_t end = redirectUrl.find_first_of("#/?", start);
            if (end == std::string::npos) end = redirectUrl.length();
            
            std::string setIdStr = redirectUrl.substr(start, end - start);
            LogInfo("Resolved BeatmapSet ID: " + setIdStr);
            return std::wstring(setIdStr.begin(), setIdStr.end());
        }
    } else {
            LogError("Failed to resolve redirect: " + error);
    }
    return L"";
}
