#ifndef GESTURETYPES_H
#define GESTURETYPES_H

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "src/core/tracking/HandTrackingTypes.h"

struct GestureConfig {
    double pinchEnterRatio{0.25};
    double pinchExitRatio{0.35};
    std::int64_t pinchEnterHoldUs{75'000};
    std::int64_t pinchExitHoldUs{75'000};
    std::int64_t trackingLostTimeoutUs{150'000};
    double fingerExtendedMaxCurl{0.22};
    double fingerCurledMinCurl{0.38};
    double thumbExtendedMaxCurl{0.30};
    double thumbMinSpreadRatio{0.50};
    double handScaleEpsilon{1e-6};

    bool operator==(const GestureConfig&) const = default;
};

inline GestureConfig sanitizeGestureConfig(GestureConfig config) {
    const GestureConfig defaults;
    if (!std::isfinite(config.pinchEnterRatio) ||
        !std::isfinite(config.pinchExitRatio) ||
        config.pinchEnterRatio <= 0.0 ||
        config.pinchExitRatio <= config.pinchEnterRatio) {
        config.pinchEnterRatio = defaults.pinchEnterRatio;
        config.pinchExitRatio = defaults.pinchExitRatio;
    }
    if (config.pinchEnterHoldUs < 0)
        config.pinchEnterHoldUs = defaults.pinchEnterHoldUs;
    if (config.pinchExitHoldUs < 0)
        config.pinchExitHoldUs = defaults.pinchExitHoldUs;
    if (config.trackingLostTimeoutUs < 0)
        config.trackingLostTimeoutUs = defaults.trackingLostTimeoutUs;
    if (!std::isfinite(config.fingerExtendedMaxCurl) ||
        config.fingerExtendedMaxCurl < 0.0)
        config.fingerExtendedMaxCurl = defaults.fingerExtendedMaxCurl;
    if (!std::isfinite(config.fingerCurledMinCurl) ||
        config.fingerCurledMinCurl <= config.fingerExtendedMaxCurl)
        config.fingerCurledMinCurl = defaults.fingerCurledMinCurl;
    if (!std::isfinite(config.thumbExtendedMaxCurl) ||
        config.thumbExtendedMaxCurl < 0.0)
        config.thumbExtendedMaxCurl = defaults.thumbExtendedMaxCurl;
    if (!std::isfinite(config.thumbMinSpreadRatio) ||
        config.thumbMinSpreadRatio <= 0.0)
        config.thumbMinSpreadRatio = defaults.thumbMinSpreadRatio;
    if (!std::isfinite(config.handScaleEpsilon) || config.handScaleEpsilon <= 0.0)
        config.handScaleEpsilon = defaults.handScaleEpsilon;
    return config;
}

enum class StaticGesture {
    NONE,
    OPEN_HAND,
    POINTING,
    PINCH,
};

struct GestureObservation {
    bool valid{false};
    StaticGesture pose{StaticGesture::NONE};
    bool pointerActive{false};
    bool pinchActive{false};
    double pinchRatio{std::numeric_limits<double>::infinity()};
    Handedness handedness{Handedness::UNKNOWN};
    std::uint64_t frameId{0};
    std::int64_t timestampUs{0};
    Point3D pointerPoint{};
};

enum class GestureEventType {
    POINTER_ACTIVE,
    POINTER_INACTIVE,
    PINCH_BEGIN,
    PINCH_END,
    PINCH_CANCEL,
};

struct GestureEvent {
    GestureEventType type{GestureEventType::POINTER_INACTIVE};
    Handedness handedness{Handedness::UNKNOWN};
    std::uint64_t frameId{0};
    std::int64_t timestampUs{0};
    Point3D pointer{};
};

template <std::size_t Capacity>
struct GestureEventBuffer {
    std::array<GestureEvent, Capacity> events{};
    std::size_t count{0};

    void push(const GestureEvent& event) {
        if (count < events.size()) events[count++] = event;
    }
};

using GestureEventBatch = GestureEventBuffer<4>;

#endif // GESTURETYPES_H
