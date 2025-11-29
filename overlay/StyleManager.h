#pragma once
#include "imgui.h"

class StyleManager {
public:
    static StyleManager& Instance();
    void ApplyTheme();
    void LoadFonts();

private:
    StyleManager() = default;
    ~StyleManager() = default;
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
};
