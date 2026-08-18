#ifndef HANDTRACKINGTYPES_H
#define HANDTRACKINGTYPES_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "src/core/gestures/Landmarks.h"

enum class Handedness { UNKNOWN, LEFT, RIGHT };

struct TrackedHand {
    std::array<Point3D, 21> landmarks{};
    std::optional<std::array<Point3D, 21>> worldLandmarks;
    Handedness handedness{Handedness::UNKNOWN};
    float handednessScore{0.0F};
};

struct HandTrackingFrame {
    std::vector<TrackedHand> hands;
    std::int64_t timestampUs{0};
    std::uint64_t frameId{0};
    bool valid{false};
};

struct RgbImageView {
    const std::uint8_t* data{nullptr};
    int width{0};
    int height{0};
    std::size_t rowStride{0};

    bool isValid() const {
        return data != nullptr && width > 0 && height > 0 &&
               rowStride >= static_cast<std::size_t>(width * 3);
    }
};

struct HandTrackingConfig {
    std::string modelPath{"models/hand_landmarker.task"};
    int numHands{2};
    float minHandDetectionConfidence{0.5F};
    float minHandPresenceConfidence{0.5F};
    float minTrackingConfidence{0.5F};
};

#endif // HANDTRACKINGTYPES_H
