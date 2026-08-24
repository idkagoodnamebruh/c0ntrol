#ifndef INMEMORYSETTINGSSTORE_H
#define INMEMORYSETTINGSSTORE_H

#include <utility>

#include "src/core/config/ISettingsStore.h"

class InMemorySettingsStore final : public ISettingsStore {
public:
    explicit InMemorySettingsStore(SettingsMap values = {})
        : m_values(std::move(values)) {}

    SettingsLoadResult load() override {
        const ConfigDecodeResult decoded = decodeRuntimeConfig(m_values);
        return {decoded.config, decoded.usedDefaults, decoded.migrated,
                decoded.warning};
    }

    bool save(const RuntimeConfig& config, std::string& error) override {
        error.clear();
        m_values = encodeRuntimeConfig(config);
        return true;
    }

    bool resetToDefaults(std::string& error) override {
        return save(RuntimeConfig{}, error);
    }

    const SettingsMap& rawValues() const { return m_values; }

private:
    SettingsMap m_values;
};

#endif // INMEMORYSETTINGSSTORE_H
