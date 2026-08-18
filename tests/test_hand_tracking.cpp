#include <cassert>
#include <iostream>

#include "src/core/tracking/LegacyLandmarksAdapter.h"
#include "src/core/tracking/MockHandTrackingBackend.h"
#include "src/core/tracking/TrackingClock.h"

int main() {
    HandTrackingFrame defaults;
    assert(!defaults.valid && defaults.hands.empty());

    TrackedHand left;
    left.handedness = Handedness::LEFT;
    left.handednessScore = 0.75F;
    left.landmarks[8] = Point3D(0.2, 0.3, 0.4);
    assert(left.landmarks.size() == 21);
    assert(left.handedness == Handedness::LEFT);

    HandTrackingFrame zero;
    zero.valid = true;
    assert(toLegacyLandmarks(zero).points.empty());

    TrackedHand right;
    right.handedness = Handedness::RIGHT;
    right.landmarks[8] = Point3D(0.8, 0.7, 0.6);
    HandTrackingFrame two;
    two.valid = true;
    two.hands = {left, right};
    auto legacy = toLegacyLandmarks(two);
    assert(legacy.points.size() == 21 && legacy.points[8].x == 0.8);

    TrackingClock clock;
    const auto id0 = clock.nextFrameId();
    const auto id1 = clock.nextFrameId();
    const auto time0 = clock.nextTimestampUs();
    const auto time1 = clock.nextTimestampUs();
    assert(id1 > id0 && time1 > time0);

    MockHandTrackingBackend mock;
    auto failed = mock.process({}, 1, 1);
    assert(!failed.valid && !mock.lastError().empty());
    assert(mock.initialize({}));
    auto generated = mock.process({}, 2, 3);
    assert(generated.valid && generated.hands.size() == 1);
    assert(generated.hands[0].landmarks.size() == 21);
    assert(generated.timestampUs == 2 && generated.frameId == 3);
    mock.shutdown();

    std::cout << "[PASS] test_hand_tracking" << std::endl;
}
