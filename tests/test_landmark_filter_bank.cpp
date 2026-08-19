#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "src/core/filters/LandmarkFilterBank.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

void requireNear(double actual, double expected, double tolerance,
                 const char* message) {
    require(std::abs(actual - expected) <= tolerance, message);
}

double variance(const std::vector<double>& values) {
    double mean = 0.0;
    for (double value : values) mean += value;
    mean /= static_cast<double>(values.size());
    double sum = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        sum += delta * delta;
    }
    return sum / static_cast<double>(values.size());
}

TrackedHand makeHand(Handedness handedness, double base,
                     bool includeWorld = false) {
    TrackedHand hand;
    hand.handedness = handedness;
    hand.handednessScore = 0.9F;
    for (std::size_t i = 0; i < hand.landmarks.size(); ++i) {
        hand.landmarks[i] = {base + i * 0.001, 0.25 + i * 0.002,
                             -0.05 + i * 0.0005};
    }
    if (includeWorld) {
        std::array<Point3D, 21> world{};
        for (std::size_t i = 0; i < world.size(); ++i) {
            world[i] = {base * 0.1 + i * 0.0001, 0.02 + i * 0.0002,
                        -0.01 + i * 0.0001};
        }
        hand.worldLandmarks = world;
    }
    return hand;
}

TrackedHand makeZeroHand(Handedness handedness) {
    TrackedHand hand;
    hand.handedness = handedness;
    hand.handednessScore = 0.8F;
    return hand;
}

HandTrackingFrame makeFrame(std::int64_t timestampUs,
                            std::vector<TrackedHand> hands,
                            std::uint64_t frameId = 0,
                            bool valid = true) {
    HandTrackingFrame frame;
    frame.timestampUs = timestampUs;
    frame.frameId = frameId;
    frame.valid = valid;
    frame.hands = std::move(hands);
    return frame;
}

LandmarkFilterConfig testConfig() {
    LandmarkFilterConfig config;
    config.normalized = {1.0, 0.0, 1.0, 1.0};
    config.world = {1.0, 0.0, 1.0, 1.0};
    config.teleportThreshold = 10.0;
    config.handResetTimeoutUs = 400'000;
    return config;
}

void testStableHandsAndMetadata() {
    LandmarkFilterBank bank(testConfig());
    auto rawRight = makeFrame(0, {makeHand(Handedness::RIGHT, 0.60)}, 10);
    const double originalRawX = rawRight.hands[0].landmarks[0].x;
    auto firstRight = bank.process(rawRight);
    auto secondRight = bank.process(
        makeFrame(16'667, {makeHand(Handedness::RIGHT, 0.62)}, 11));
    requireNear(firstRight.hands[0].landmarks[0].x, 0.60, 1e-12,
                "first RIGHT sample is raw");
    require(secondRight.hands[0].landmarks[0].x > 0.60 &&
                secondRight.hands[0].landmarks[0].x < 0.62,
            "stable RIGHT hand is filtered");
    require(rawRight.hands[0].landmarks[0].x == originalRawX,
            "input raw frame is not modified");
    require(secondRight.timestampUs == 16'667 && secondRight.frameId == 11 &&
                secondRight.valid,
            "frame metadata is preserved");
    require(secondRight.hands[0].handedness == Handedness::RIGHT &&
                secondRight.hands[0].handednessScore == 0.9F,
            "hand metadata is copied without filtering");

    bank.reset();
    auto firstLeft = bank.process(
        makeFrame(20'000, {makeHand(Handedness::LEFT, 0.20)}));
    auto secondLeft = bank.process(
        makeFrame(36'667, {makeHand(Handedness::LEFT, 0.22)}));
    requireNear(firstLeft.hands[0].landmarks[0].x, 0.20, 1e-12,
                "first LEFT sample is raw");
    require(secondLeft.hands[0].landmarks[0].x > 0.20 &&
                secondLeft.hands[0].landmarks[0].x < 0.22,
            "stable LEFT hand is filtered");
}

void testHandOrderSwapAndAxisIsolation() {
    LandmarkFilterBank bank(testConfig());
    bank.process(makeFrame(0, {makeHand(Handedness::LEFT, 0.20),
                               makeHand(Handedness::RIGHT, 0.80)}));
    auto swapped = bank.process(
        makeFrame(16'667, {makeHand(Handedness::RIGHT, 0.82),
                           makeHand(Handedness::LEFT, 0.22)}));
    require(swapped.hands[0].handedness == Handedness::RIGHT &&
                swapped.hands[0].landmarks[0].x > 0.80 &&
                swapped.hands[0].landmarks[0].x < 0.82,
            "RIGHT state follows handedness when order swaps");
    require(swapped.hands[1].handedness == Handedness::LEFT &&
                swapped.hands[1].landmarks[0].x > 0.20 &&
                swapped.hands[1].landmarks[0].x < 0.22,
            "LEFT state follows handedness when order swaps");

    LandmarkFilterBank independent(testConfig());
    independent.process(makeFrame(0, {makeZeroHand(Handedness::RIGHT)}));
    auto changed = makeZeroHand(Handedness::RIGHT);
    changed.landmarks[0].x = 1.0;
    auto result = independent.process(makeFrame(20'000, {changed}));
    require(result.hands[0].landmarks[0].x > 0.0 &&
                result.hands[0].landmarks[0].x < 1.0,
            "changed scalar channel is filtered");
    requireNear(result.hands[0].landmarks[0].y, 0.0, 1e-12,
                "X does not contaminate Y");
    requireNear(result.hands[0].landmarks[0].z, 0.0, 1e-12,
                "X does not contaminate Z");
    requireNear(result.hands[0].landmarks[1].x, 0.0, 1e-12,
                "landmark 0 does not contaminate landmark 1");
}

void testAmbiguousHandednessPolicies() {
    LandmarkFilterBank unknownBank(testConfig());
    unknownBank.process(makeFrame(0, {makeHand(Handedness::RIGHT, 0.10)}));
    auto unknown = unknownBank.process(
        makeFrame(10'000, {makeHand(Handedness::UNKNOWN, 0.50)}));
    requireNear(unknown.hands[0].landmarks[0].x, 0.50, 1e-12,
                "UNKNOWN hand passes through raw");
    auto afterUnknown = unknownBank.process(
        makeFrame(20'000, {makeHand(Handedness::RIGHT, 0.55)}));
    requireNear(afterUnknown.hands[0].landmarks[0].x, 0.55, 1e-12,
                "UNKNOWN resets named hand states before reuse");

    LandmarkFilterBank duplicateBank(testConfig());
    duplicateBank.process(makeFrame(0, {makeHand(Handedness::LEFT, 0.10)}));
    auto duplicate = duplicateBank.process(
        makeFrame(10'000, {makeHand(Handedness::LEFT, 0.30),
                           makeHand(Handedness::LEFT, 0.70)}));
    requireNear(duplicate.hands[0].landmarks[0].x, 0.30, 1e-12,
                "first duplicate LEFT passes through raw");
    requireNear(duplicate.hands[1].landmarks[0].x, 0.70, 1e-12,
                "second duplicate LEFT passes through raw");
    auto afterDuplicate = duplicateBank.process(
        makeFrame(20'000, {makeHand(Handedness::LEFT, 0.40)}));
    requireNear(afterDuplicate.hands[0].landmarks[0].x, 0.40, 1e-12,
                "duplicate handedness resets its slot");
}

void testEmptyInvalidTimeoutAndTeleport() {
    auto config = testConfig();
    config.handResetTimeoutUs = 100'000;
    LandmarkFilterBank bank(config);
    bank.process(makeFrame(0, {makeHand(Handedness::RIGHT, 0.10)}));

    auto empty = bank.process(makeFrame(50'000, {}, 20));
    require(empty.valid && empty.hands.empty() && empty.frameId == 20,
            "valid zero-hand frame remains valid and empty");

    auto invalid = bank.process(
        makeFrame(75'000, {makeHand(Handedness::RIGHT, 0.30)}, 21, false));
    require(!invalid.valid && invalid.hands.empty() && invalid.frameId == 21,
            "invalid frame remains invalid and contains no hands");

    bank.process(makeFrame(150'000, {}));
    auto afterTimeout = bank.process(
        makeFrame(160'000, {makeHand(Handedness::RIGHT, 0.15)}));
    requireNear(afterTimeout.hands[0].landmarks[0].x, 0.15, 1e-12,
                "hand reappearance after timeout starts from raw");

    auto teleportConfig = testConfig();
    teleportConfig.teleportThreshold = 0.10;
    LandmarkFilterBank teleportBank(teleportConfig);
    teleportBank.process(makeFrame(0, {makeHand(Handedness::RIGHT, 0.10)}));
    auto teleported = teleportBank.process(
        makeFrame(20'000, {makeHand(Handedness::RIGHT, 0.60)}));
    requireNear(teleported.hands[0].landmarks[0].x, 0.60, 1e-12,
                "wrist teleport resets the hand filters");

    LandmarkFilterBank repeatedTime(testConfig());
    repeatedTime.process(makeFrame(100, {makeHand(Handedness::LEFT, 0.10)}));
    auto repeated = repeatedTime.process(
        makeFrame(100, {makeHand(Handedness::LEFT, 0.20)}));
    requireNear(repeated.hands[0].landmarks[0].x, 0.20, 1e-12,
                "repeated frame timestamp resets the hand state");
}

void testWorldIsolationAndPassthrough() {
    LandmarkFilterBank bank(testConfig());
    auto firstHand = makeHand(Handedness::RIGHT, 0.20, true);
    const double originalWorldX = (*firstHand.worldLandmarks)[0].x;
    bank.process(makeFrame(0, {firstHand}));

    auto secondHand = makeHand(Handedness::RIGHT, 0.22, true);
    (*secondHand.worldLandmarks)[0].x = originalWorldX;
    auto second = bank.process(makeFrame(20'000, {secondHand}));
    require(second.hands[0].worldLandmarks.has_value(),
            "world landmarks remain present");
    requireNear((*second.hands[0].worldLandmarks)[0].x, originalWorldX, 1e-12,
                "normalized movement does not contaminate world state");

    auto absent = bank.process(
        makeFrame(40'000, {makeHand(Handedness::RIGHT, 0.24, false)}));
    require(!absent.hands[0].worldLandmarks.has_value(),
            "absent world landmarks remain absent");

    auto disabledConfig = testConfig();
    disabledConfig.enabled = false;
    LandmarkFilterBank disabled(disabledConfig);
    auto raw = makeFrame(0, {makeHand(Handedness::RIGHT, 0.33)});
    auto passthrough = disabled.process(raw);
    requireNear(passthrough.hands[0].landmarks[8].x,
                raw.hands[0].landmarks[8].x, 1e-12,
                "disabled filter is raw passthrough");
    disabled.setEnabled(true);
    auto firstEnabled = disabled.process(
        makeFrame(20'000, {makeHand(Handedness::RIGHT, 0.40)}));
    requireNear(firstEnabled.hands[0].landmarks[0].x, 0.40, 1e-12,
                "enabling starts with clean state");
}

void testDeterministicJitterAndAdaptiveMotion() {
    auto config = testConfig();
    config.normalized.beta = 0.0;
    LandmarkFilterBank bank(config);
    const double offsets[] = {-0.010, 0.008, -0.006, 0.011, -0.009, 0.005,
                              -0.007, 0.009};
    std::vector<double> rawValues;
    std::vector<double> filteredValues;
    for (int i = 0; i < 120; ++i) {
        const double sample = 0.5 + offsets[i % 8];
        auto raw = makeFrame(i * 16'667,
                             {makeHand(Handedness::RIGHT, sample)});
        rawValues.push_back(raw.hands[0].landmarks[8].x);
        filteredValues.push_back(bank.process(raw).hands[0].landmarks[8].x);
    }
    require(variance(filteredValues) < variance(rawValues),
            "filter bank reduces deterministic landmark jitter");

    auto lowConfig = testConfig();
    lowConfig.normalized.beta = 0.0;
    auto highConfig = testConfig();
    highConfig.normalized.beta = 1.0;
    LandmarkFilterBank lowBeta(lowConfig);
    LandmarkFilterBank highBeta(highConfig);
    lowBeta.process(makeFrame(0, {makeHand(Handedness::RIGHT, 0.30)}));
    highBeta.process(makeFrame(0, {makeHand(Handedness::RIGHT, 0.30)}));
    const double slow = lowBeta.process(
        makeFrame(20'000, {makeHand(Handedness::RIGHT, 0.80)}))
                            .hands[0].landmarks[0].x;
    const double fast = highBeta.process(
        makeFrame(20'000, {makeHand(Handedness::RIGHT, 0.80)}))
                             .hands[0].landmarks[0].x;
    require(fast > slow, "positive beta adapts faster to rapid movement");
}

} // namespace

int main() {
    testStableHandsAndMetadata();
    testHandOrderSwapAndAxisIsolation();
    testAmbiguousHandednessPolicies();
    testEmptyInvalidTimeoutAndTeleport();
    testWorldIsolationAndPassthrough();
    testDeterministicJitterAndAdaptiveMotion();
    std::cout << "[PASS] test_landmark_filter_bank\n";
    return 0;
}
