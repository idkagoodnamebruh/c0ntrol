#include "GestureEngine.h"

#include <cmath>

GestureEngine::GestureEngine(GestureConfig config)
    : m_config(sanitizeGestureConfig(config)) {}

GestureObservation GestureEngine::observe(
    const HandFeatures& features, std::uint64_t frameId,
    std::int64_t timestampUs) const {
    GestureObservation observation;
    observation.handedness = features.handedness;
    observation.frameId = frameId;
    observation.timestampUs = timestampUs;
    if (!features.valid || timestampUs < 0) return observation;

    observation.valid = true;
    observation.pointerPoint = features.pointerPoint;
    observation.pinchRatio = features.pinchRatio;
    observation.pinchActive =
        std::isfinite(features.pinchRatio) &&
        features.pinchRatio <= m_config.pinchEnterRatio;

    const bool pointing =
        features.indexExtended &&
        features.middleCurl >= m_config.fingerCurledMinCurl &&
        features.ringCurl >= m_config.fingerCurledMinCurl &&
        features.pinkyCurl >= m_config.fingerCurledMinCurl;

    const int extendedLongFingers =
        static_cast<int>(features.indexExtended) +
        static_cast<int>(features.middleExtended) +
        static_cast<int>(features.ringExtended) +
        static_cast<int>(features.pinkyExtended);
    const bool openHand = !pointing && extendedLongFingers >= 3;

    observation.pointerActive = pointing;
    if (pointing) observation.pose = StaticGesture::POINTING;
    else if (openHand) observation.pose = StaticGesture::OPEN_HAND;
    else if (observation.pinchActive) observation.pose = StaticGesture::PINCH;
    return observation;
}
