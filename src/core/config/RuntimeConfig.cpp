#include "RuntimeConfig.h"

#include <cmath>

CameraConfig sanitizeCameraConfig(CameraConfig config) {
    const CameraConfig defaults;
    if (config.index < 0) config.index = defaults.index;
    if (config.requestedWidth < 16 || config.requestedWidth > 16'384)
        config.requestedWidth = defaults.requestedWidth;
    if (config.requestedHeight < 16 || config.requestedHeight > 16'384)
        config.requestedHeight = defaults.requestedHeight;
    if (!std::isfinite(config.requestedFps) || config.requestedFps <= 0.0 ||
        config.requestedFps > 1'000.0) {
        config.requestedFps = defaults.requestedFps;
    }
    if (config.requestedBufferSize < 1 ||
        config.requestedBufferSize > 64) {
        config.requestedBufferSize = defaults.requestedBufferSize;
    }
    return config;
}

RuntimeConfig sanitizeRuntimeConfig(RuntimeConfig config) {
    config.configVersion = kRuntimeConfigVersion;
    config.camera = sanitizeCameraConfig(config.camera);
    config.pointer = sanitizePointerMappingConfig(config.pointer);
    config.filtering = sanitizeLandmarkFilterConfig(config.filtering);
    config.gestures = sanitizeGestureConfig(config.gestures);
    config.input = sanitizeInputConfig(config.input);
    return config;
}
