#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

#include "src/core/tracking/MediaPipeHandTrackingBackend.h"

int main() {
    MediaPipeHandTrackingBackend backend;
    HandTrackingConfig config;
    config.modelPath = C0NTROL_MODEL_PATH;
    assert(backend.initialize(config));

    // A deterministic generated RGB image validates Create + DetectForVideo
    // without claiming that a hand exists in the test input.
    constexpr int width = 256;
    constexpr int height = 256;
    std::vector<std::uint8_t> rgb(width * height * 3, 127);
    const RgbImageView image{rgb.data(), width, height, width * 3};
    auto first = backend.process(image, 1'000, 0);
    auto second = backend.process(image, 1'001, 1); // same truncated ms; backend must advance it
    assert(first.valid && second.valid);
    for (const auto& hand : first.hands) {
        assert(hand.landmarks.size() == 21);
        assert(hand.handedness == Handedness::UNKNOWN ||
               hand.handedness == Handedness::LEFT ||
               hand.handedness == Handedness::RIGHT);
    }
    backend.shutdown();
    backend.shutdown();
    std::cout << "[PASS] test_mediapipe_hand_tracking_backend" << std::endl;
}
