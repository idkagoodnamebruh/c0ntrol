#include <cstdlib>
#include <iostream>

#include "src/core/config/InMemorySettingsStore.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

} // namespace

int main() {
    RuntimeConfig configured;
    configured.camera = {3, 1280, 720, 60.0, 2};
    configured.pointer = {true, true, 0.1, 0.2, 0.15, 0.05};
    configured.filtering.normalized.minCutoff = 1.7;
    configured.filtering.normalized.beta = 0.2;
    configured.filtering.enabled = false;
    configured.gestures.pinchEnterRatio = 0.2;
    configured.gestures.pinchExitRatio = 0.4;
    configured.gestures.pinchEnterHoldUs = 0;
    configured.input = {true, Handedness::LEFT};
    configured = sanitizeRuntimeConfig(configured);

    InMemorySettingsStore store;
    std::string error;
    require(store.save(configured, error) && error.empty(),
            "settings save succeeds");
    const SettingsLoadResult roundTrip = store.load();
    require(roundTrip.config == configured,
            "all RuntimeConfig fields round-trip");
    require(store.rawValues().at("configVersion") == "1",
            "schema version is persisted");

    SettingsMap missing{{"configVersion", "1"}, {"camera/index", "5"}};
    const ConfigDecodeResult missingResult = decodeRuntimeConfig(missing);
    require(missingResult.config.camera.index == 5 &&
                missingResult.config.camera.requestedWidth == 640 &&
                !missingResult.config.input.enabled &&
                missingResult.usedDefaults,
            "missing keys preserve present values and use safe defaults");

    SettingsMap invalid = encodeRuntimeConfig(configured);
    invalid["camera/index"] = "-9";
    invalid["camera/fps"] = "nan";
    invalid["pointer/leftMargin"] = "0.8";
    invalid["pointer/rightMargin"] = "0.7";
    invalid["filter/normalized/minCutoff"] = "inf";
    invalid["filter/normalized/beta"] = "-1";
    invalid["gesture/pinchEnterRatio"] = "0.6";
    invalid["gesture/pinchExitRatio"] = "0.2";
    invalid["input/preferredHand"] = "UNKNOWN";
    const ConfigDecodeResult invalidResult = decodeRuntimeConfig(invalid);
    require(invalidResult.config.camera.index == 0 &&
                invalidResult.config.camera.requestedFps == 30.0,
            "invalid camera and non-finite values are sanitized");
    require(invalidResult.config.pointer.leftMargin == 0.0 &&
                invalidResult.config.pointer.rightMargin == 0.0,
            "invalid combined margins are rejected");
    require(invalidResult.config.filtering.normalized.minCutoff > 0.0 &&
                invalidResult.config.filtering.normalized.beta >= 0.0,
            "invalid OneEuro values are sanitized");
    require(invalidResult.config.gestures.pinchEnterRatio <
                invalidResult.config.gestures.pinchExitRatio &&
                invalidResult.config.input.preferredHand == Handedness::RIGHT &&
                !invalidResult.warning.empty(),
            "corruption produces safe config and one warning summary");

    SettingsMap old{{"configVersion", "0"},
                    {"camera/index", "4"},
                    {"input/enabled", "true"}};
    const ConfigDecodeResult migrated = decodeRuntimeConfig(old);
    require(migrated.migrated && migrated.config.configVersion == 1 &&
                migrated.config.camera.index == 4 &&
                migrated.config.input.enabled,
            "old schema reads known keys and migrates to version 1");

    SettingsMap unknown = encodeRuntimeConfig(configured);
    unknown["future/unknown"] = "ignored";
    require(decodeRuntimeConfig(unknown).config == configured,
            "unknown keys are ignored");

    SettingsMap future{{"configVersion", "99"},
                       {"input/enabled", "true"}};
    const ConfigDecodeResult futureResult = decodeRuntimeConfig(future);
    require(futureResult.usedDefaults &&
                !futureResult.config.input.enabled &&
                !futureResult.warning.empty(),
            "future schema fails safely to disabled-input defaults");

    require(store.resetToDefaults(error), "reset defaults persists");
    require(store.load().config == RuntimeConfig{},
            "reset restores complete canonical defaults");

    std::cout << "[PASS] test_settings_serialization\n";
    return 0;
}
