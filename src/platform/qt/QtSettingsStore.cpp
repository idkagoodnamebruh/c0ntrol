#include "QtSettingsStore.h"

#include <QSettings>
#include <QString>

namespace {

constexpr const char* kRuntimeGroup = "runtime";

std::string settingsError(QSettings::Status status) {
    switch (status) {
        case QSettings::AccessError:
            return "settings storage access error";
        case QSettings::FormatError:
            return "settings storage format error";
        default:
            return {};
    }
}

} // namespace

SettingsLoadResult QtSettingsStore::load() {
    QSettings settings;
    SettingsMap values;
    settings.beginGroup(kRuntimeGroup);
    for (const QString& key : settings.allKeys()) {
        values[key.toStdString()] = settings.value(key).toString().toStdString();
    }
    settings.endGroup();

    const ConfigDecodeResult decoded = decodeRuntimeConfig(values);
    SettingsLoadResult result{decoded.config, decoded.usedDefaults,
                              decoded.migrated, decoded.warning};
    const std::string storageError = settingsError(settings.status());
    if (!storageError.empty()) {
        result.config = RuntimeConfig{};
        result.usedDefaults = true;
        result.warning = storageError + "; safe defaults used";
        return result;
    }

    if (values.empty() || result.migrated || result.usedDefaults) {
        std::string saveError;
        if (!save(result.config, saveError)) {
            if (!result.warning.empty()) result.warning += "; ";
            result.warning += saveError;
        }
    }
    return result;
}

bool QtSettingsStore::save(const RuntimeConfig& config, std::string& error) {
    QSettings settings;
    settings.beginGroup(kRuntimeGroup);
    settings.remove(QString());
    const SettingsMap values = encodeRuntimeConfig(config);
    for (const auto& [key, value] : values) {
        settings.setValue(QString::fromStdString(key),
                          QString::fromStdString(value));
    }
    settings.endGroup();
    settings.sync();
    error = settingsError(settings.status());
    return error.empty();
}

bool QtSettingsStore::resetToDefaults(std::string& error) {
    return save(RuntimeConfig{}, error);
}
