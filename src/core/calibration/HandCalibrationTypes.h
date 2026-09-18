#ifndef HANDCALIBRATIONTYPES_H
#define HANDCALIBRATIONTYPES_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "src/core/gestures/HandFeatures.h"

// Stable explicit IDs. Add future poses without renumbering these values.
enum class HandCalibrationPose : std::uint16_t {
    OPEN_HAND = 1,
    POINTING = 2,
    PINCH_OPEN = 3,
    PINCH_CLOSED = 4,
};

inline constexpr std::array<HandCalibrationPose, 4> kHandCalibrationPoses{
    HandCalibrationPose::OPEN_HAND, HandCalibrationPose::POINTING,
    HandCalibrationPose::PINCH_OPEN, HandCalibrationPose::PINCH_CLOSED};
inline constexpr std::uint32_t kHandCalibrationProfileVersion = 1;

struct HandCalibrationObservation {
    HandCalibrationPose pose{HandCalibrationPose::OPEN_HAND};
    // features.handedness is the single source of handedness; no duplicate label.
    HandFeatures features{};
    std::optional<double> normalizedLuma;
    // Optional caller-provided metadata, not an inferred quality score.
    std::optional<double> trackingConfidence;
};

struct HandCalibrationStatistics {
    std::size_t sampleCount{0};
    // Zero when empty; sampleCount == 0 means unavailable, not a measurement.
    double p10{0.0};
    double median{0.0};
    double p90{0.0};
    bool operator==(const HandCalibrationStatistics&) const = default;
};

struct HandCalibrationConfig {
    std::size_t minimumSamplesPerPose{10};
    std::size_t maximumSamplesPerPose{256};
    double minimumPinchMedianSeparation{0.10};
    double minimumPinchQuantileGap{0.04};
    double minimumCurlMedianSeparation{0.15};
    double minimumCurlQuantileGap{0.05};
};

enum class HandCalibrationSampleResult {
    ACCEPTED,
    INVALID_CONFIG,
    UNSUPPORTED_POSE,
    INVALID_FEATURES,
    INVALID_HANDEDNESS,
    OPPOSITE_HAND,
    INVALID_LUMINANCE,
    INVALID_CONFIDENCE,
    POSE_CAPACITY_REACHED,
};

enum class HandCalibrationProfileStatus {
    INSUFFICIENT_SAMPLES,
    INVALID_CONFIG,
    PINCH_NOT_SEPARABLE,
    CURL_NOT_SEPARABLE,
    VALID,
};

struct HandCalibrationPoseStatistics {
    std::size_t sampleCount{0};
    // Kept per pose: thumb semantics differ between open hand and pinching.
    HandCalibrationStatistics thumbCurl;
    HandCalibrationStatistics thumbSpreadRatio;
    bool operator==(const HandCalibrationPoseStatistics&) const = default;
};

struct HandCalibrationProfile {
    std::uint32_t version{kHandCalibrationProfileVersion};
    bool valid{false};
    HandCalibrationProfileStatus status{
        HandCalibrationProfileStatus::INSUFFICIENT_SAMPLES};
    Handedness handedness{Handedness::UNKNOWN};
    std::size_t acceptedSamples{0};
    std::size_t rejectedSamples{0};
    // Indexed by kHandCalibrationPoses, never by casting an enum to an index.
    std::array<HandCalibrationPoseStatistics, kHandCalibrationPoses.size()> poses{};
    HandCalibrationStatistics handScale;
    HandCalibrationStatistics pinchClosed;
    HandCalibrationStatistics pinchOpen;
    HandCalibrationStatistics extendedCurl;
    HandCalibrationStatistics curledCurl;
    HandCalibrationStatistics luminance;
    HandCalibrationStatistics trackingConfidence;
    // All four are absent unless the whole profile is valid. Never applied here.
    std::optional<double> recommendedPinchEnterRatio;
    std::optional<double> recommendedPinchExitRatio;
    std::optional<double> recommendedFingerExtendedMaxCurl;
    std::optional<double> recommendedFingerCurledMinCurl;
    bool operator==(const HandCalibrationProfile&) const = default;
};

#endif // HANDCALIBRATIONTYPES_H
