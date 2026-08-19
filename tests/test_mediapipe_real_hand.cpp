#include <cassert>
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
    assert(input && magic == "P6" && width > 0 && height > 0 && maxValue == 255);
    input.get();
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width * height * 3));
    input.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
    assert(input.gcount() == static_cast<std::streamsize>(pixels.size()));
    return pixels;
}

int main() {
    MediaPipeHandTrackingBackend backend;
    HandTrackingConfig config;
    config.modelPath = C0NTROL_MODEL_PATH;
    assert(backend.initialize(config));
    int width = 0, height = 0;
    auto rgb = loadPpm(C0NTROL_HAND_ASSET_PATH, width, height);
    auto result = backend.process({rgb.data(), width, height,
                                   static_cast<std::size_t>(width * 3)},
                                  1'000, 0);
    assert(result.valid && !result.hands.empty());
    const auto& hand = result.hands.front();
    bool anyNonZero = false;
    for (const auto& point : hand.landmarks) {
        assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
        anyNonZero |= point.x != 0.0 || point.y != 0.0 || point.z != 0.0;
    }
    assert(anyNonZero);
    assert(hand.handedness == Handedness::LEFT || hand.handedness == Handedness::RIGHT);
    assert(hand.handednessScore >= 0.0F && hand.handednessScore <= 1.0F);
    if (hand.worldLandmarks) {
        for (const auto& point : *hand.worldLandmarks)
            assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
    }
    std::cout << "hands=" << result.hands.size()
              << " handedness=" << (hand.handedness == Handedness::LEFT ? "LEFT" : "RIGHT")
              << " score=" << hand.handednessScore
              << " world=" << (hand.worldLandmarks ? "21" : "ABSENT") << '\n';
    backend.shutdown();
}
