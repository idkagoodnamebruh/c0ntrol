#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "src/core/config/RuntimeConfig.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

} // namespace

int main() {
    const RuntimeConfig defaults;
    require(defaults.configVersion == 1, "schema default is version 1");
    require(!defaults.input.enabled,
            "native input is disabled on first run");
    require(defaults.camera.index == 0 &&
                defaults.camera.requestedWidth == 640 &&
                defaults.camera.requestedHeight == 480 &&
                defaults.camera.requestedFps == 30.0 &&
                defaults.camera.requestedBufferSize == 1,
            "camera defaults have one canonical source");

    RuntimeConfig invalid;
    invalid.configVersion = 99;
    invalid.camera = {-5, 1, 99'999,
                      std::numeric_limits<double>::infinity(), 0};
    invalid.pointer.leftMargin = -1.0;
    invalid.pointer.rightMargin = std::numeric_limits<double>::quiet_NaN();
    invalid.pointer.topMargin = 0.8;
    invalid.pointer.bottomMargin = 0.7;
    invalid.filtering.normalized.minCutoff = 0.0;
    invalid.filtering.normalized.beta = -1.0;
    invalid.filtering.world.derivativeCutoff =
        std::numeric_limits<double>::quiet_NaN();
    invalid.filtering.handResetTimeoutUs = 0;
    invalid.filtering.teleportThreshold = -1.0;
    invalid.gestures.pinchEnterRatio = 0.5;
    invalid.gestures.pinchExitRatio = 0.4;
    invalid.gestures.pinchEnterHoldUs = -1;
    invalid.gestures.pinchExitHoldUs = -1;
    invalid.input.preferredHand = Handedness::UNKNOWN;

    const RuntimeConfig sane = sanitizeRuntimeConfig(invalid);
    require(sane.configVersion == kRuntimeConfigVersion,
            "schema is normalized to current version");
    require(sane.camera == defaults.camera,
            "invalid camera values use canonical defaults");
    require(sane.pointer.leftMargin == 0.0 &&
                sane.pointer.rightMargin == 0.0 &&
                sane.pointer.topMargin == 0.0 &&
                sane.pointer.bottomMargin == 0.0,
            "invalid or degenerate margins are sanitized");
    require(sane.filtering.normalized.minCutoff > 0.0 &&
                sane.filtering.normalized.beta >= 0.0 &&
                sane.filtering.world.derivativeCutoff > 0.0 &&
                sane.filtering.handResetTimeoutUs > 0 &&
                sane.filtering.teleportThreshold > 0.0,
            "OneEuro and filter values are finite and valid");
    require(sane.gestures.pinchEnterRatio <
                sane.gestures.pinchExitRatio &&
                sane.gestures.pinchEnterHoldUs >= 0 &&
                sane.gestures.pinchExitHoldUs >= 0,
            "gesture hysteresis and hold times are valid");
    require(sane.input.preferredHand == Handedness::RIGHT,
            "invalid hand uses canonical preference");

    RuntimeConfig immediate;
    immediate.gestures.pinchEnterHoldUs = 0;
    immediate.gestures.pinchExitHoldUs = 0;
    immediate.gestures.trackingLostTimeoutUs = 0;
    const RuntimeConfig allowed = sanitizeRuntimeConfig(immediate);
    require(allowed.gestures.pinchEnterHoldUs == 0 &&
                allowed.gestures.pinchExitHoldUs == 0 &&
                allowed.gestures.trackingLostTimeoutUs == 0,
            "zero-duration gesture configuration is accepted");

    std::cout << "[PASS] test_runtime_config\n";
    return 0;
}
