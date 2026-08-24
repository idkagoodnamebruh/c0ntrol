#ifndef DYNAMICGESTURERECOGNIZER_H
#define DYNAMICGESTURERECOGNIZER_H

#include <cstdint>
#include <deque>
#include <optional>

#include "src/core/gestures/DynamicGestureConfig.h"
#include "src/core/gestures/GestureTypes.h"
#include "src/core/gestures/HandFeatures.h"

class DynamicGestureRecognizer {
public:
    explicit DynamicGestureRecognizer(
        Handedness handedness,
        DynamicGestureConfig config = {});

    std::optional<GestureEvent> update(
        const HandFeatures& features,
        StaticGesture pose,
        std::uint64_t frameId,
        std::int64_t timestampUs);
    void reset();

    const DynamicGestureConfig& config() const { return m_config; }

private:
    struct Sample {
        Point3D palmCenter{};
        double handScale{0.0};
        std::int64_t timestampUs{0};
    };

    bool validSample(const HandFeatures& features, StaticGesture pose,
                     std::int64_t timestampUs) const;
    void seed(const HandFeatures& features, std::int64_t timestampUs);
    double referenceHandScale() const;

    Handedness m_handedness;
    DynamicGestureConfig m_config;
    std::deque<Sample> m_samples;
    std::int64_t m_lastTimestampUs{0};
    std::int64_t m_cooldownUntilUs{0};
    bool m_hasTimestamp{false};
};

#endif // DYNAMICGESTURERECOGNIZER_H
