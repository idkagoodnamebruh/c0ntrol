#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>

#include "src/core/gestures/DynamicGestureRecognizer.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

DynamicGestureConfig testConfig() {
    DynamicGestureConfig config;
    config.swipeMinDistanceHandScales = 1.0;
    config.swipeMinVelocityHandScalesPerSecond = 4.0;
    config.directionDominanceRatio = 1.5;
    config.swipeMaxDurationUs = 500'000;
    config.maxSampleGapUs = 150'000;
    config.cooldownUs = 300'000;
    config.minimumSamples = 3;
    return config;
}

HandFeatures sample(double x, double y, double scale = 0.1,
                    Handedness hand = Handedness::RIGHT) {
    HandFeatures value;
    value.valid = true;
    value.handedness = hand;
    value.palmCenter = {x, y, 0.0};
    value.pointerPoint = value.palmCenter;
    value.handScale = scale;
    return value;
}

std::optional<GestureEvent> update(
    DynamicGestureRecognizer& recognizer, double x, double y,
    std::int64_t timestampUs, double scale = 0.1,
    StaticGesture pose = StaticGesture::OPEN_HAND,
    Handedness hand = Handedness::RIGHT) {
    return recognizer.update(sample(x, y, scale, hand), pose,
                             static_cast<std::uint64_t>(timestampUs + 10),
                             timestampUs);
}

std::optional<GestureEvent> fastGesture(double endX, double endY,
                                        double scale = 0.1) {
    DynamicGestureRecognizer recognizer(Handedness::RIGHT, testConfig());
    require(!update(recognizer, 0.5, 0.5, 0, scale),
            "first sample does not emit");
    require(!update(recognizer,
                    0.5 + (endX - 0.5) * 0.45,
                    0.5 + (endY - 0.5) * 0.45,
                    45'000, scale),
            "intermediate sample does not emit");
    return update(recognizer, endX, endY, 100'000, scale);
}

void testThresholdsAndDirections() {
    DynamicGestureRecognizer stationary(Handedness::RIGHT, testConfig());
    update(stationary, 0.5, 0.5, 0);
    update(stationary, 0.5, 0.5, 50'000);
    require(!update(stationary, 0.5, 0.5, 100'000),
            "stationary OPEN_HAND is NONE");

    DynamicGestureRecognizer small(Handedness::RIGHT, testConfig());
    update(small, 0.5, 0.5, 0);
    update(small, 0.53, 0.5, 50'000);
    require(!update(small, 0.56, 0.5, 100'000),
            "small motion is NONE");

    DynamicGestureRecognizer slow(Handedness::RIGHT, testConfig());
    for (int i = 0; i <= 5; ++i) {
        require(!update(slow, 0.5 + 0.03 * i, 0.5, i * 100'000),
                "large slow movement is NONE");
    }

    const auto right = fastGesture(0.64, 0.5);
    require(right && right->type == GestureEventType::SWIPE_RIGHT,
            "fast right swipe is recognized");
    const auto left = fastGesture(0.36, 0.5);
    require(left && left->type == GestureEventType::SWIPE_LEFT,
            "fast left swipe is recognized");
    const auto up = fastGesture(0.5, 0.36);
    require(up && up->type == GestureEventType::SWIPE_UP,
            "fast up swipe is recognized");
    const auto down = fastGesture(0.5, 0.64);
    require(down && down->type == GestureEventType::SWIPE_DOWN,
            "fast down swipe is recognized");
    require(!fastGesture(0.64, 0.64),
            "ambiguous diagonal is NONE");
}

void testScaleAndTimestampInvariance() {
    for (double scale : {0.05, 0.1, 0.2}) {
        const auto event = fastGesture(0.5 + 1.4 * scale, 0.5, scale);
        require(event && event->type == GestureEventType::SWIPE_RIGHT,
                "classification is invariant across hand scales");
    }

    DynamicGestureRecognizer variable(Handedness::RIGHT, testConfig());
    update(variable, 0.50, 0.50, 0);
    update(variable, 0.54, 0.50, 17'000);
    update(variable, 0.58, 0.50, 61'000);
    const auto event = update(variable, 0.64, 0.50, 113'000);
    require(event && event->type == GestureEventType::SWIPE_RIGHT,
            "variable frame intervals preserve classification");
}

void testPoseGateAndCooldown() {
    for (StaticGesture pose : {StaticGesture::POINTING,
                               StaticGesture::PINCH}) {
        DynamicGestureRecognizer gated(Handedness::RIGHT, testConfig());
        update(gated, 0.50, 0.50, 0, 0.1, pose);
        update(gated, 0.57, 0.50, 50'000, 0.1, pose);
        require(!update(gated, 0.64, 0.50, 100'000, 0.1, pose),
                "non-OPEN_HAND motion cannot swipe");
    }

    DynamicGestureRecognizer recognizer(Handedness::RIGHT, testConfig());
    update(recognizer, 0.50, 0.50, 0);
    update(recognizer, 0.57, 0.50, 50'000);
    require(update(recognizer, 0.64, 0.50, 100'000).has_value(),
            "first physical swipe emits once");
    require(!update(recognizer, 0.71, 0.50, 150'000) &&
                !update(recognizer, 0.78, 0.50, 200'000) &&
                !update(recognizer, 0.85, 0.50, 250'000),
            "continued movement is suppressed during cooldown");
    update(recognizer, 0.50, 0.50, 410'000);
    update(recognizer, 0.57, 0.50, 460'000);
    require(update(recognizer, 0.64, 0.50, 510'000).has_value(),
            "new movement after cooldown emits a second swipe");
}

void testResetAndInvalidTime() {
    DynamicGestureRecognizer loss(Handedness::RIGHT, testConfig());
    update(loss, 0.50, 0.50, 0);
    update(loss, 0.57, 0.50, 50'000);
    HandFeatures missing;
    loss.update(missing, StaticGesture::NONE, 3, 75'000);
    require(!update(loss, 0.64, 0.50, 100'000),
            "tracking loss resets history");

    DynamicGestureRecognizer gap(Handedness::RIGHT, testConfig());
    update(gap, 0.50, 0.50, 0);
    update(gap, 0.57, 0.50, 50'000);
    require(!update(gap, 0.64, 0.50, 250'001),
            "timestamp gap resets history");

    DynamicGestureRecognizer invalidTime(Handedness::RIGHT, testConfig());
    update(invalidTime, 0.50, 0.50, 100'000);
    update(invalidTime, 0.57, 0.50, 150'000);
    require(!update(invalidTime, 0.64, 0.50, 150'000),
            "repeated timestamp is safe");
    require(!update(invalidTime, 0.70, 0.50, 140'000),
            "regressive timestamp is safe");

    DynamicGestureRecognizer invalidScale(Handedness::RIGHT, testConfig());
    require(!update(invalidScale, 0.5, 0.5, 0,
                    std::numeric_limits<double>::quiet_NaN()),
            "NaN hand scale is safe");
    require(!update(invalidScale, 0.6, 0.5, 50'000, 0.0),
            "degenerate hand scale is safe");
}

void testHandIsolationAndAmbiguity() {
    DynamicGestureRecognizer left(Handedness::LEFT, testConfig());
    DynamicGestureRecognizer right(Handedness::RIGHT, testConfig());
    update(left, 0.50, 0.50, 0, 0.1, StaticGesture::OPEN_HAND,
           Handedness::LEFT);
    update(right, 0.50, 0.50, 0);
    update(left, 0.43, 0.50, 50'000, 0.1, StaticGesture::OPEN_HAND,
           Handedness::LEFT);
    update(right, 0.57, 0.50, 50'000);
    const auto leftEvent = update(left, 0.36, 0.50, 100'000, 0.1,
                                  StaticGesture::OPEN_HAND, Handedness::LEFT);
    const auto rightEvent = update(right, 0.64, 0.50, 100'000);
    require(leftEvent && leftEvent->type == GestureEventType::SWIPE_LEFT &&
                leftEvent->handedness == Handedness::LEFT,
            "LEFT history is independent");
    require(rightEvent && rightEvent->type == GestureEventType::SWIPE_RIGHT &&
                rightEvent->handedness == Handedness::RIGHT,
            "RIGHT history is independent");

    DynamicGestureRecognizer conservative(Handedness::RIGHT, testConfig());
    update(conservative, 0.50, 0.50, 0);
    update(conservative, 0.57, 0.50, 50'000);
    require(!update(conservative, 0.60, 0.50, 75'000, 0.1,
                    StaticGesture::OPEN_HAND, Handedness::UNKNOWN),
            "UNKNOWN identity resets nominal history");
    require(!update(conservative, 0.64, 0.50, 100'000),
            "identity ambiguity cannot complete another hand's swipe");
}

} // namespace

int main() {
    testThresholdsAndDirections();
    testScaleAndTimestampInvariance();
    testPoseGateAndCooldown();
    testResetAndInvalidTime();
    testHandIsolationAndAmbiguity();
    std::cout << "[PASS] test_dynamic_gestures\n";
    return 0;
}
