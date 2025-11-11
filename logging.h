#pragma once
#include <iostream>
#include <string>

// Logging helper functions
inline void LogDebug(const std::string& msg) {
#ifdef _DEBUG
    std::cout << "[DEBUG] " << msg << std::endl;
#endif
}

inline void LogInfo(const std::string& msg) {
    #ifdef _DEBUG
        std::cout << "[INFO] " << msg << std::endl;
    #endif
}

inline void LogError(const std::string& msg) {
    // Always log errors regardless of debug mode
    std::cout << "[ERROR] " << msg << std::endl;
}

inline void LogWarning(const std::string& msg) {
    std::cout << "[WARNING] " << msg << std::endl;
}