#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Provider.h"

using ProviderFactory = std::function<std::unique_ptr<Provider>()>;

class ProviderRegistry {
public:
    static ProviderRegistry& Instance();

    bool Register(const std::string& name, ProviderFactory factory);
    std::vector<std::string> GetProviderNames() const;
    std::unique_ptr<Provider> CreateProvider(int index) const;
    std::unique_ptr<Provider> CreateProvider(const std::string& name) const;

private:
    ProviderRegistry() = default;
    std::vector<std::pair<std::string, ProviderFactory>> m_Providers;
};
