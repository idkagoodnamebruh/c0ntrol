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
    configured.dynamicGestures.swipeMinDistanceHandScales = 1.7;
    configured.dynamicGestures.swipeMinVelocityHandScalesPerSecond = 5.5;
    configured.dynamicGestures.cooldownUs = 275'000;
    configured.input.swipeScrollEnabled = false;
    configured.input.scrollNotchesPerSwipe = 7;
    configured.input.invertSwipeScroll = true;
    configured = sanitizeRuntimeConfig(configured);

    InMemorySettingsStore store;
    std::string error;
    require(store.save(configured, error) && error.empty(),
            "settings save succeeds");
    const SettingsLoadResult roundTrip = store.load();
    require(roundTrip.config == configured,
            "all RuntimeConfig fields round-trip");
    require(store.rawValues().at("configVersion") == "2",
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
    invalid["dynamic/swipeMinDistanceHandScales"] = "nan";
    invalid["dynamic/swipeMinVelocityHandScalesPerSecond"] = "0";
    invalid["dynamic/directionDominanceRatio"] = "1";
    invalid["dynamic/minimumSamples"] = "1";
    invalid["input/preferredHand"] = "UNKNOWN";
    invalid["input/scrollNotchesPerSwipe"] = "100";
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
                invalidResult.config.dynamicGestures
                        .swipeMinDistanceHandScales ==
                    DynamicGestureConfig{}.swipeMinDistanceHandScales &&
                invalidResult.config.dynamicGestures
                        .swipeMinVelocityHandScalesPerSecond ==
                    DynamicGestureConfig{}
                        .swipeMinVelocityHandScalesPerSecond &&
                invalidResult.config.dynamicGestures
                        .directionDominanceRatio ==
                    DynamicGestureConfig{}.directionDominanceRatio &&
                invalidResult.config.dynamicGestures.minimumSamples ==
                    DynamicGestureConfig{}.minimumSamples &&
                invalidResult.config.input.preferredHand == Handedness::RIGHT &&
                invalidResult.config.input.scrollNotchesPerSwipe == 3 &&
                !invalidResult.warning.empty(),
            "corruption produces safe config and one warning summary");

    SettingsMap old{{"configVersion", "0"},
                    {"camera/index", "4"},
                    {"input/enabled", "true"}};
    const ConfigDecodeResult migrated = decodeRuntimeConfig(old);
    require(migrated.migrated && migrated.config.configVersion == 2 &&
                migrated.config.camera.index == 4 &&
                migrated.config.input.enabled,
            "old schema reads known keys and migrates to version 2");

    SettingsMap version1 = encodeRuntimeConfig(configured);
    version1["configVersion"] = "1";
    version1.erase("dynamic/enabled");
    version1.erase("dynamic/swipeMinDistanceHandScales");
    version1.erase("dynamic/swipeMinVelocityHandScalesPerSecond");
    version1.erase("dynamic/directionDominanceRatio");
    version1.erase("dynamic/swipeMaxDurationUs");
    version1.erase("dynamic/maxSampleGapUs");
    version1.erase("dynamic/cooldownUs");
    version1.erase("dynamic/minimumSamples");
    version1.erase("input/swipeScrollEnabled");
    version1.erase("input/scrollNotchesPerSwipe");
    version1.erase("input/invertSwipeScroll");
    const ConfigDecodeResult migratedV1 = decodeRuntimeConfig(version1);
    RuntimeConfig expectedV1 = configured;
    expectedV1.dynamicGestures = DynamicGestureConfig{};
    expectedV1.input.swipeScrollEnabled =
        InputConfig{}.swipeScrollEnabled;
    expectedV1.input.scrollNotchesPerSwipe =
        InputConfig{}.scrollNotchesPerSwipe;
    expectedV1.input.invertSwipeScroll =
        InputConfig{}.invertSwipeScroll;
    expectedV1.configVersion = 2;
    require(migratedV1.migrated &&
                migratedV1.config == expectedV1 &&
                migratedV1.config.camera == configured.camera &&
                migratedV1.config.pointer == configured.pointer &&
                migratedV1.config.filtering == configured.filtering &&
                migratedV1.config.gestures == configured.gestures &&
                migratedV1.config.input.enabled == configured.input.enabled &&
                migratedV1.config.input.preferredHand ==
                    configured.input.preferredHand,
            "schema v1 preserves Phase 7 values and defaults only v2 fields");

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
