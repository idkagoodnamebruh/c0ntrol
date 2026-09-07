#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "src/core/tracking/MediaPipeHandTrackingBackend.h"

static std::vector<std::uint8_t> loadPpm(const char* path, int& width, int& height) {
    std::ifstream input(path, std::ios::binary);
    std::string magic;
    int maxValue = 0;
    input >> magic >> width >> height >> maxValue;
    if (!input || magic != "P6" || width <= 0 || height <= 0 || maxValue != 255)
        return {};
    input.get();
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 3));
    input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
    if (input.gcount() != static_cast<std::streamsize>(pixels.size()))
        return {};
    return pixels;
}

static int fail(const std::string& message) {
    std::cerr << "[FAIL] test_mediapipe_real_hand: " << message << '\n';
    return 1;
}

int main() {
    MediaPipeHandTrackingBackend backend;
    HandTrackingConfig config;
    config.modelPath = C0NTROL_MODEL_PATH;
    if (!backend.initialize(config))
        return fail("initialize: " + backend.lastError());
    int width = 0, height = 0;
    auto rgb = loadPpm(C0NTROL_HAND_ASSET_PATH, width, height);
    if (rgb.empty())
        return fail("invalid PPM test asset");
    auto result = backend.process({rgb.data(), width, height,
                                   static_cast<std::size_t>(width * 3)},
                                  1'000, 0);
    if (!result.valid)
        return fail("DetectForVideo: " + backend.lastError());
    if (result.hands.empty())
        return fail("DetectForVideo returned zero hands");
    const auto& hand = result.hands.front();
    if (hand.landmarks.size() != 21)
        return fail("first hand did not contain 21 landmarks");
    bool anyNonZero = false;
    for (const auto& point : hand.landmarks) {
        if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
            !std::isfinite(point.z))
            return fail("non-finite normalized landmark");
        anyNonZero |= point.x != 0.0 || point.y != 0.0 || point.z != 0.0;
    }
    if (!anyNonZero)
        return fail("all normalized landmarks were zero");
    if (hand.handedness != Handedness::LEFT && hand.handedness != Handedness::RIGHT)
        return fail("handedness was not LEFT or RIGHT");
    if (hand.handednessScore < 0.0F || hand.handednessScore > 1.0F)
        return fail("handedness score was outside [0, 1]");
    if (hand.worldLandmarks) {
        if (hand.worldLandmarks->size() != 21)
            return fail("world landmarks did not contain 21 points");
        for (const auto& point : *hand.worldLandmarks) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
                !std::isfinite(point.z))
                return fail("non-finite world landmark");
        }
    }
    std::cout << "hands=" << result.hands.size()
              << " handedness=" << (hand.handedness == Handedness::LEFT ? "LEFT" : "RIGHT")
              << " score=" << hand.handednessScore
              << " world=" << (hand.worldLandmarks ? "21" : "ABSENT") << '\n';
    backend.shutdown();
    backend.shutdown();
    return 0;
}
