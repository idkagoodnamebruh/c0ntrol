#include "HandFeatureExtractor.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

bool finitePoint(const Point3D& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z);
}

double jointAngle(const Point3D& a, const Point3D& b, const Point3D& c,
                  double minimumSegment, bool& valid) {
    const double abx = a.x - b.x;
    const double aby = a.y - b.y;
    const double abz = a.z - b.z;
    const double cbx = c.x - b.x;
    const double cby = c.y - b.y;
    const double cbz = c.z - b.z;
    const double abLength = std::sqrt(abx * abx + aby * aby + abz * abz);
    const double cbLength = std::sqrt(cbx * cbx + cby * cby + cbz * cbz);
    if (abLength <= minimumSegment || cbLength <= minimumSegment) {
        valid = false;
        return 0.0;
    }
    const double cosine = std::clamp(
        (abx * cbx + aby * cby + abz * cbz) / (abLength * cbLength),
        -1.0, 1.0);
    return std::acos(cosine);
}

} // namespace

HandFeatureExtractor::HandFeatureExtractor(GestureConfig config)
    : m_config(sanitizeGestureConfig(config)) {}

double HandFeatureExtractor::computeHandScale(
    const std::array<Point3D, 21>& landmarks) {
    const double wristToMiddleMcp = landmarks[0].distanceTo(landmarks[9]);
    const double palmWidth = landmarks[5].distanceTo(landmarks[17]);
    if (!std::isfinite(wristToMiddleMcp) || !std::isfinite(palmWidth))
        return 0.0;
    return 0.5 * (wristToMiddleMcp + palmWidth);
}

double HandFeatureExtractor::fingerCurl(
    const std::array<Point3D, 21>& points, std::size_t mcp, std::size_t pip,
    std::size_t dip, std::size_t tip, double scale, bool& valid) const {
    const double minimumSegment =
        std::max(m_config.handScaleEpsilon, scale * 1e-5);
    const double pipAngle =
        jointAngle(points[mcp], points[pip], points[dip], minimumSegment, valid);
    const double dipAngle =
        jointAngle(points[pip], points[dip], points[tip], minimumSegment, valid);
    if (!valid) return 0.0;
    return std::clamp(1.0 - (pipAngle + dipAngle) / (2.0 * kPi), 0.0, 1.0);
}

HandFeatures HandFeatureExtractor::extract(const TrackedHand& hand) const {
    HandFeatures features;
    features.handedness = hand.handedness;

    for (const auto& point : hand.landmarks) {
        if (!finitePoint(point)) return features;
    }

    features.handScale = computeHandScale(hand.landmarks);
    if (!std::isfinite(features.handScale) ||
        features.handScale <= m_config.handScaleEpsilon) {
        return features;
    }

    const auto& points = hand.landmarks;
    features.palmCenter = {
        (points[0].x + points[5].x + points[9].x + points[17].x) / 4.0,
        (points[0].y + points[5].y + points[9].y + points[17].y) / 4.0,
        (points[0].z + points[5].z + points[9].z + points[17].z) / 4.0,
    };
    features.pointerPoint = points[8];
    features.pinchRatio = points[4].distanceTo(points[8]) / features.handScale;
    if (!std::isfinite(features.pinchRatio)) return features;

    bool validGeometry = true;
    const double thumbArticularCurl =
        fingerCurl(points, 1, 2, 3, 4, features.handScale, validGeometry);
    features.indexCurl =
        fingerCurl(points, 5, 6, 7, 8, features.handScale, validGeometry);
    features.middleCurl =
        fingerCurl(points, 9, 10, 11, 12, features.handScale, validGeometry);
    features.ringCurl =
        fingerCurl(points, 13, 14, 15, 16, features.handScale, validGeometry);
    features.pinkyCurl =
        fingerCurl(points, 17, 18, 19, 20, features.handScale, validGeometry);
    if (!validGeometry) return features;

    features.thumbSpreadRatio =
        points[4].distanceTo(features.palmCenter) / features.handScale;
    const double foldedIntoPalm = std::clamp(
        (m_config.thumbMinSpreadRatio - features.thumbSpreadRatio) /
            m_config.thumbMinSpreadRatio,
        0.0, 1.0);
    features.thumbCurl =
        std::clamp(0.75 * thumbArticularCurl + 0.25 * foldedIntoPalm,
                   0.0, 1.0);

    features.thumbExtended =
        thumbArticularCurl <= m_config.thumbExtendedMaxCurl &&
        features.thumbSpreadRatio >= m_config.thumbMinSpreadRatio;
    features.indexExtended =
        features.indexCurl <= m_config.fingerExtendedMaxCurl;
    features.middleExtended =
        features.middleCurl <= m_config.fingerExtendedMaxCurl;
    features.ringExtended =
        features.ringCurl <= m_config.fingerExtendedMaxCurl;
    features.pinkyExtended =
        features.pinkyCurl <= m_config.fingerExtendedMaxCurl;
    features.valid = true;
    return features;
}
