#ifndef GESTURETESTFIXTURES_H
#define GESTURETESTFIXTURES_H

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/core/gestures/HandFeatureExtractor.h"

namespace gesture_test {

inline void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

inline void requireNear(double actual, double expected, double tolerance,
                        const char* message) {
    require(std::abs(actual - expected) <= tolerance, message);
}

inline void setExtendedFinger(std::array<Point3D, 21>& points,
                              std::size_t mcp, double x, double mcpY) {
    points[mcp] = {x, mcpY, 0.0};
    points[mcp + 1] = {x, mcpY - 0.12, 0.0};
    points[mcp + 2] = {x, mcpY - 0.23, 0.0};
    points[mcp + 3] = {x, mcpY - 0.34, 0.0};
}

inline void setCurledFinger(std::array<Point3D, 21>& points,
                            std::size_t mcp, double x, double mcpY) {
    points[mcp] = {x, mcpY, 0.0};
    points[mcp + 1] = {x, mcpY - 0.12, 0.0};
    points[mcp + 2] = {x + 0.08, mcpY - 0.06, 0.0};
    points[mcp + 3] = {x, mcpY, 0.0};
}

inline TrackedHand makeOpenHand(Handedness handedness = Handedness::RIGHT) {
    TrackedHand hand;
    hand.handedness = handedness;
    hand.handednessScore = 0.99F;
    auto& p = hand.landmarks;
    p[0] = {0.0, 0.45, 0.0};
    p[1] = {-0.08, 0.34, 0.0};
    p[2] = {-0.17, 0.30, 0.0};
    p[3] = {-0.26, 0.26, 0.0};
    p[4] = {-0.35, 0.22, 0.0};
    setExtendedFinger(p, 5, -0.12, 0.22);
    setExtendedFinger(p, 9, 0.00, 0.18);
    setExtendedFinger(p, 13, 0.10, 0.21);
    setExtendedFinger(p, 17, 0.19, 0.26);
    if (handedness == Handedness::LEFT) {
        for (auto& point : p) point.x = -point.x;
    }
    return hand;
}

inline TrackedHand makePointingHand(
    Handedness handedness = Handedness::RIGHT) {
    TrackedHand hand = makeOpenHand(handedness);
    auto& p = hand.landmarks;
    const double direction = handedness == Handedness::LEFT ? -1.0 : 1.0;
    setCurledFinger(p, 9, 0.00, 0.18);
    setCurledFinger(p, 13, direction * 0.10, 0.21);
    setCurledFinger(p, 17, direction * 0.19, 0.26);
    return hand;
}

inline void setPinchRatio(TrackedHand& hand, double ratio) {
    const double scale = HandFeatureExtractor::computeHandScale(hand.landmarks);
    hand.landmarks[4] = {hand.landmarks[8].x + ratio * scale,
                         hand.landmarks[8].y, hand.landmarks[8].z};
}

inline TrackedHand transformed(TrackedHand hand, double scale,
                               double angleDegrees,
                               Point3D translation = {0.37, 0.41, -0.12}) {
    const double radians = angleDegrees * 3.14159265358979323846 / 180.0;
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    const Point3D palmCenter{
        (hand.landmarks[0].x + hand.landmarks[5].x +
         hand.landmarks[9].x + hand.landmarks[17].x) / 4.0,
        (hand.landmarks[0].y + hand.landmarks[5].y +
         hand.landmarks[9].y + hand.landmarks[17].y) / 4.0,
        (hand.landmarks[0].z + hand.landmarks[5].z +
         hand.landmarks[9].z + hand.landmarks[17].z) / 4.0};
    for (auto& point : hand.landmarks) {
        const double x = (point.x - palmCenter.x) * scale;
        const double y = (point.y - palmCenter.y) * scale;
        point = {translation.x + cosine * x - sine * y,
                 translation.y + sine * x + cosine * y,
                 translation.z + (point.z - palmCenter.z) * scale};
    }
    return hand;
}

inline HandTrackingFrame makeFrame(std::int64_t timestampUs,
                                   std::vector<TrackedHand> hands,
                                   std::uint64_t frameId = 0,
                                   bool valid = true) {
    HandTrackingFrame frame;
    frame.valid = valid;
    frame.timestampUs = timestampUs;
    frame.frameId = frameId;
    frame.hands = std::move(hands);
    return frame;
}

} // namespace gesture_test

#endif // GESTURETESTFIXTURES_H
