#ifndef ISETTINGSSTORE_H
#define ISETTINGSSTORE_H

#include <string>

#include "src/core/config/SettingsCodec.h"

struct SettingsLoadResult {
    RuntimeConfig config{};
    bool usedDefaults{false};
    bool migrated{false};
    std::string warning;
};

class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;
    virtual SettingsLoadResult load() = 0;
    virtual bool save(const RuntimeConfig& config, std::string& error) = 0;
    virtual bool resetToDefaults(std::string& error) = 0;
};

#endif // ISETTINGSSTORE_H
