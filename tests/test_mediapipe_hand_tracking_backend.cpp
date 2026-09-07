#include <cstdint>
#include <iostream>
#include <vector>

#include "src/core/tracking/MediaPipeHandTrackingBackend.h"

int main() {
    MediaPipeHandTrackingBackend backend;
    HandTrackingConfig config;
    config.modelPath = C0NTROL_MODEL_PATH;
    if (!backend.initialize(config)) {
        std::cerr << "[FAIL] initialize: " << backend.lastError() << std::endl;
        return 1;
    }

    // A deterministic generated RGB image validates Create + DetectForVideo
    // without claiming that a hand exists in the test input.
    constexpr int width = 256;
    constexpr int height = 256;
    std::vector<std::uint8_t> rgb(width * height * 3, 127);
    const RgbImageView image{rgb.data(), width, height, width * 3};
    auto first = backend.process(image, 1'000, 0);
    auto second = backend.process(image, 1'001, 1); // same truncated ms; backend must advance it
    if (!first.valid || !second.valid) {
        std::cerr << "[FAIL] DetectForVideo: " << backend.lastError() << std::endl;
        return 1;
    }
    for (const auto& hand : first.hands) {
        if (hand.landmarks.size() != 21 ||
            (hand.handedness != Handedness::UNKNOWN &&
             hand.handedness != Handedness::LEFT &&
             hand.handedness != Handedness::RIGHT)) {
            std::cerr << "[FAIL] invalid converted hand" << std::endl;
            return 1;
        }
    }
    backend.shutdown();
    backend.shutdown();
    std::cout << "[PASS] test_mediapipe_hand_tracking_backend" << std::endl;
}
