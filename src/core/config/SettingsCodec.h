#ifndef SETTINGSCODEC_H
#define SETTINGSCODEC_H

#include <map>
#include <string>

#include "src/core/config/RuntimeConfig.h"

using SettingsMap = std::map<std::string, std::string>;

struct ConfigDecodeResult {
    RuntimeConfig config{};
    bool usedDefaults{false};
    bool migrated{false};
    std::string warning;
};

SettingsMap encodeRuntimeConfig(const RuntimeConfig& config);
ConfigDecodeResult decodeRuntimeConfig(const SettingsMap& values);

#endif // SETTINGSCODEC_H
