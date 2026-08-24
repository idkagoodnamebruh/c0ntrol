#ifndef DYNAMICGESTURECONFIG_H
#define DYNAMICGESTURECONFIG_H

#include <cmath>
#include <cstddef>
#include <cstdint>

struct DynamicGestureConfig {
    bool enabled{true};
    double swipeMinDistanceHandScales{1.25};
    double swipeMinVelocityHandScalesPerSecond{4.0};
    double directionDominanceRatio{1.5};
    std::int64_t swipeMaxDurationUs{500'000};
    std::int64_t maxSampleGapUs{150'000};
    std::int64_t cooldownUs{400'000};
    std::size_t minimumSamples{3};

    bool operator==(const DynamicGestureConfig&) const = default;
};

inline DynamicGestureConfig sanitizeDynamicGestureConfig(
    DynamicGestureConfig config) {
    const DynamicGestureConfig defaults;
    if (!std::isfinite(config.swipeMinDistanceHandScales) ||
        config.swipeMinDistanceHandScales <= 0.0) {
        config.swipeMinDistanceHandScales =
            defaults.swipeMinDistanceHandScales;
    }
    if (!std::isfinite(config.swipeMinVelocityHandScalesPerSecond) ||
        config.swipeMinVelocityHandScalesPerSecond <= 0.0) {
        config.swipeMinVelocityHandScalesPerSecond =
            defaults.swipeMinVelocityHandScalesPerSecond;
    }
    if (!std::isfinite(config.directionDominanceRatio) ||
        config.directionDominanceRatio <= 1.0) {
        config.directionDominanceRatio = defaults.directionDominanceRatio;
    }
    if (config.swipeMaxDurationUs <= 0)
        config.swipeMaxDurationUs = defaults.swipeMaxDurationUs;
    if (config.maxSampleGapUs <= 0)
        config.maxSampleGapUs = defaults.maxSampleGapUs;
    if (config.cooldownUs < 0)
        config.cooldownUs = defaults.cooldownUs;
    if (config.minimumSamples < 2)
        config.minimumSamples = defaults.minimumSamples;
    return config;
}

#endif // DYNAMICGESTURECONFIG_H
