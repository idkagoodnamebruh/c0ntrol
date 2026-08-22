#ifndef CAPTURETYPES_H
#define CAPTURETYPES_H

#include <cstdint>

enum class CaptureState {
    STOPPED,
    STARTING,
    RUNNING,
    FAILED,
    STOPPING,
};

struct CapturedFrameMetadata {
    std::uint64_t captureSequence{0};
    std::int64_t captureTimestampUs{0};
};

template <typename T>
struct CapturedFrame {
    T value;
    CapturedFrameMetadata metadata;
};

struct CaptureMetrics {
    double captureFps{0.0};
    std::uint64_t capturedFrames{0};
    std::uint64_t overwrittenFrames{0};
    std::uint64_t captureFailures{0};
    CaptureState state{CaptureState::STOPPED};
};

#endif // CAPTURETYPES_H
