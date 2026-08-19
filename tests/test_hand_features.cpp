#include <iostream>
#include <limits>

#include "tests/GestureTestFixtures.h"
#include "src/core/gestures/GestureEngine.h"

using namespace gesture_test;

int main() {
    HandFeatureExtractor extractor;
    GestureEngine engine;
    const HandFeatures right = extractor.extract(makeOpenHand(Handedness::RIGHT));
    const HandFeatures left = extractor.extract(makeOpenHand(Handedness::LEFT));
    require(right.valid && left.valid, "LEFT and RIGHT geometry is valid");
    require(right.handScale > 1e-6 && std::isfinite(right.handScale),
            "hand scale is finite and non-degenerate");
    require(right.thumbExtended && left.thumbExtended,
            "thumb extension is symmetric for LEFT and RIGHT");
    requireNear(right.thumbCurl, left.thumbCurl, 1e-12,
                "mirroring does not change thumb curl");
    require(right.indexExtended && right.middleExtended &&
                right.ringExtended && right.pinkyExtended,
            "straight fingers have low curl");

    const HandFeatures pointing = extractor.extract(makePointingHand());
    require(pointing.indexExtended && !pointing.middleExtended &&
                !pointing.ringExtended && !pointing.pinkyExtended,
            "curled fingers are separated from extended index");

    for (double scale : {0.5, 1.0, 2.0}) {
        auto pinching = makePointingHand();
        setPinchRatio(pinching, 0.20);
        const HandFeatures features =
            extractor.extract(transformed(pinching, scale, 0.0));
        require(features.valid, "scaled hand remains valid");
        requireNear(features.pinchRatio, 0.20, 1e-9,
                    "pinch ratio is scale invariant");
        const GestureObservation observed = engine.observe(features, 1, 1);
        require(observed.pinchActive && observed.pose == StaticGesture::POINTING,
                "scale preserves static pose and activates pinch");
    }

    for (double angle : {0.0, 45.0, 90.0}) {
        const HandFeatures features =
            extractor.extract(transformed(makePointingHand(), 1.0, angle));
        require(features.valid, "rotated hand remains valid");
        require(engine.observe(features, 1, 1).pose == StaticGesture::POINTING,
                "classification is invariant to in-plane rotation");
    }

    TrackedHand degenerate;
    degenerate.handedness = Handedness::RIGHT;
    require(!extractor.extract(degenerate).valid,
            "zero-scale geometry is rejected");
    auto nanHand = makeOpenHand();
    nanHand.landmarks[8].x = std::numeric_limits<double>::quiet_NaN();
    require(!extractor.extract(nanHand).valid, "NaN is rejected");
    auto infinityHand = makeOpenHand();
    infinityHand.landmarks[4].z = std::numeric_limits<double>::infinity();
    require(!extractor.extract(infinityHand).valid, "infinity is rejected");
    std::cout << "[PASS] test_hand_features\n";
    return 0;
}
