#pragma once
#include <string>

class OverlayTab {
public:
    virtual ~OverlayTab() = default;
    virtual std::string GetName() const = 0;
    virtual void Render() = 0;
};
