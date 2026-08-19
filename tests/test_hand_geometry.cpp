#include <iostream>

#include "tests/GestureTestFixtures.h"
#include "src/core/gestures/GestureEngine.h"

using namespace gesture_test;

int main() {
    HandFeatureExtractor extractor;
    GestureEngine engine;
    const HandFeatures open = extractor.extract(makeOpenHand());
    require(open.valid, "open-hand geometry is valid");
    require(engine.observe(open, 1, 1).pose == StaticGesture::OPEN_HAND,
            "open hand is classified from joint geometry");

    auto pointingHand = makePointingHand();
    const HandFeatures pointing = extractor.extract(pointingHand);
    require(pointing.valid && pointing.indexExtended,
            "pointing index remains extended");
    require(engine.observe(pointing, 2, 2).pose == StaticGesture::POINTING,
            "pointing pose is classified from curls");

    setPinchRatio(pointingHand, 0.20);
    const auto pinch = engine.observe(extractor.extract(pointingHand), 3, 3);
    require(pinch.pose == StaticGesture::POINTING && pinch.pinchActive,
            "pinch is an independent signal while pointing");
    std::cout << "[PASS] test_hand_geometry\n";
    return 0;
}
