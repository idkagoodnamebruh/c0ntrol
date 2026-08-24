#ifndef RUNTIMECONFIG_H
#define RUNTIMECONFIG_H

#include "src/core/actions/PointerMapper.h"
#include "src/core/config/InputConfig.h"
#include "src/core/filters/LandmarkFilterBank.h"
#include "src/core/gestures/GestureTypes.h"

inline constexpr int kRuntimeConfigVersion = 1;

struct CameraConfig {
    int index{0};
    int requestedWidth{640};
    int requestedHeight{480};
    double requestedFps{30.0};
    int requestedBufferSize{1};

    bool operator==(const CameraConfig&) const = default;
};

struct RuntimeConfig {
    int configVersion{kRuntimeConfigVersion};
    CameraConfig camera{};
    PointerMappingConfig pointer{};
    LandmarkFilterConfig filtering{};
    GestureConfig gestures{};
    InputConfig input{};

    bool operator==(const RuntimeConfig&) const = default;
};

CameraConfig sanitizeCameraConfig(CameraConfig config);
RuntimeConfig sanitizeRuntimeConfig(RuntimeConfig config);

#endif // RUNTIMECONFIG_H
