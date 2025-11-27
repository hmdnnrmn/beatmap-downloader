#include "ProviderRegistry.h"

ProviderRegistry& ProviderRegistry::Instance() {
    static ProviderRegistry instance;
    return instance;
}

bool ProviderRegistry::Register(const std::string& name, ProviderFactory factory) {
    m_Providers.emplace_back(name, factory);
    return true;
}

std::vector<std::string> ProviderRegistry::GetProviderNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_Providers) {
        names.push_back(pair.first);
    }
    return names;
}

std::unique_ptr<Provider> ProviderRegistry::CreateProvider(int index) const {
    if (index >= 0 && static_cast<size_t>(index) < m_Providers.size()) {
        return m_Providers[index].second();
    }
    return nullptr;
}

std::unique_ptr<Provider> ProviderRegistry::CreateProvider(const std::string& name) const {
    for (const auto& pair : m_Providers) {
        if (pair.first == name) {
            return pair.second();
        }
    }
    return nullptr;
}
