#include "DynamicGestureRecognizer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace {

bool finitePoint(const Point3D& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z);
}

} // namespace

DynamicGestureRecognizer::DynamicGestureRecognizer(
    Handedness handedness, DynamicGestureConfig config)
    : m_handedness(handedness),
      m_config(sanitizeDynamicGestureConfig(config)) {}

bool DynamicGestureRecognizer::validSample(
    const HandFeatures& features, StaticGesture pose,
    std::int64_t timestampUs) const {
    return m_config.enabled && timestampUs >= 0 && features.valid &&
           features.handedness == m_handedness &&
           pose == StaticGesture::OPEN_HAND &&
           finitePoint(features.palmCenter) &&
           std::isfinite(features.handScale) && features.handScale > 0.0;
}

void DynamicGestureRecognizer::seed(
    const HandFeatures& features, std::int64_t timestampUs) {
    m_samples.push_back({features.palmCenter, features.handScale, timestampUs});
    m_lastTimestampUs = timestampUs;
    m_hasTimestamp = true;
}

double DynamicGestureRecognizer::referenceHandScale() const {
    std::vector<double> scales;
    scales.reserve(m_samples.size());
    for (const auto& sample : m_samples) scales.push_back(sample.handScale);
    const auto middle = scales.begin() + scales.size() / 2;
    std::nth_element(scales.begin(), middle, scales.end());
    if (scales.size() % 2 != 0) return *middle;
    const double upper = *middle;
    const double lower = *std::max_element(scales.begin(), middle);
    return 0.5 * (lower + upper);
}

std::optional<GestureEvent> DynamicGestureRecognizer::update(
    const HandFeatures& features, StaticGesture pose,
    std::uint64_t frameId, std::int64_t timestampUs) {
    if (!validSample(features, pose, timestampUs)) {
        reset();
        return std::nullopt;
    }

    if (m_hasTimestamp && timestampUs <= m_lastTimestampUs) {
        reset();
        seed(features, timestampUs);
        return std::nullopt;
    }
    if (m_cooldownUntilUs > 0) {
        m_lastTimestampUs = timestampUs;
        m_hasTimestamp = true;
        if (timestampUs < m_cooldownUntilUs) {
            m_samples.clear();
            return std::nullopt;
        }
        m_cooldownUntilUs = 0;
        m_samples.clear();
        seed(features, timestampUs);
        return std::nullopt;
    }
    if (m_hasTimestamp &&
        timestampUs - m_lastTimestampUs > m_config.maxSampleGapUs) {
        reset();
        seed(features, timestampUs);
        return std::nullopt;
    }

    m_lastTimestampUs = timestampUs;
    m_hasTimestamp = true;

    m_samples.push_back({features.palmCenter, features.handScale, timestampUs});
    while (!m_samples.empty() &&
           timestampUs - m_samples.front().timestampUs >
               m_config.swipeMaxDurationUs) {
        m_samples.pop_front();
    }
    if (m_samples.size() < m_config.minimumSamples) return std::nullopt;

    const Sample& first = m_samples.front();
    const Sample& last = m_samples.back();
    const std::int64_t durationUs = last.timestampUs - first.timestampUs;
    if (durationUs <= 0 || durationUs > m_config.swipeMaxDurationUs)
        return std::nullopt;

    const double scale = referenceHandScale();
    if (!std::isfinite(scale) || scale <= 0.0) {
        reset();
        return std::nullopt;
    }
    const double dx = (last.palmCenter.x - first.palmCenter.x) / scale;
    const double dy = (last.palmCenter.y - first.palmCenter.y) / scale;
    if (!std::isfinite(dx) || !std::isfinite(dy)) {
        reset();
        return std::nullopt;
    }

    const double absX = std::abs(dx);
    const double absY = std::abs(dy);
    GestureEventType eventType;
    double dominantDistance = 0.0;
    if (absX > m_config.directionDominanceRatio * absY) {
        eventType = dx > 0.0 ? GestureEventType::SWIPE_RIGHT
                             : GestureEventType::SWIPE_LEFT;
        dominantDistance = absX;
    } else if (absY > m_config.directionDominanceRatio * absX) {
        eventType = dy > 0.0 ? GestureEventType::SWIPE_DOWN
                             : GestureEventType::SWIPE_UP;
        dominantDistance = absY;
    } else {
        return std::nullopt;
    }

    const double seconds = static_cast<double>(durationUs) / 1'000'000.0;
    const double velocity = dominantDistance / seconds;
    if (dominantDistance < m_config.swipeMinDistanceHandScales ||
        !std::isfinite(velocity) ||
        velocity < m_config.swipeMinVelocityHandScalesPerSecond) {
        return std::nullopt;
    }

    GestureEvent event;
    event.type = eventType;
    event.handedness = m_handedness;
    event.frameId = frameId;
    event.timestampUs = timestampUs;
    event.pointer = features.palmCenter;
    m_samples.clear();
    if (m_config.cooldownUs >
        std::numeric_limits<std::int64_t>::max() - timestampUs) {
        m_cooldownUntilUs = std::numeric_limits<std::int64_t>::max();
    } else {
        m_cooldownUntilUs = timestampUs + m_config.cooldownUs;
    }
    return event;
}

void DynamicGestureRecognizer::reset() {
    m_samples.clear();
    m_lastTimestampUs = 0;
    m_cooldownUntilUs = 0;
    m_hasTimestamp = false;
}
