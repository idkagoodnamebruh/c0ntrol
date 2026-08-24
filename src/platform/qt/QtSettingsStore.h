#ifndef QTSETTINGSSTORE_H
#define QTSETTINGSSTORE_H

#include "src/core/config/ISettingsStore.h"

class QtSettingsStore final : public ISettingsStore {
public:
    SettingsLoadResult load() override;
    bool save(const RuntimeConfig& config, std::string& error) override;
    bool resetToDefaults(std::string& error) override;
};

#endif // QTSETTINGSSTORE_H
