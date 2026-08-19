#include <iostream>

#include "tests/GestureTestFixtures.h"
#include "src/core/gestures/GesturePipeline.h"

using namespace gesture_test;

namespace {
TrackedHand pointingWithRatio(double ratio,
                              Handedness hand = Handedness::RIGHT) {
    TrackedHand result = makePointingHand(hand);
    setPinchRatio(result, ratio);
    return result;
}
} // namespace

int main() {
    GesturePipeline pipeline;
    auto open = pipeline.process(makeFrame(0, {makeOpenHand()}, 1));
    require(open.observationCount == 1 &&
                open.observations[0].pose == StaticGesture::OPEN_HAND,
            "pipeline observes OPEN_HAND");

    auto pointing = pipeline.process(
        makeFrame(10'000, {pointingWithRatio(0.60)}, 2));
    require(pointing.events.count == 1 &&
                pointing.events.events[0].type ==
                    GestureEventType::POINTER_ACTIVE,
            "pipeline emits POINTER_ACTIVE once");

    require(pipeline.process(makeFrame(20'000,
        {pointingWithRatio(0.20)}, 3)).events.count == 0,
            "pinch entry is debounced");
    auto begin = pipeline.process(
        makeFrame(95'000, {pointingWithRatio(0.20)}, 4));
    require(begin.events.count == 1 &&
                begin.events.events[0].type == GestureEventType::PINCH_BEGIN,
            "pipeline emits one PINCH_BEGIN");
    require(pipeline.process(makeFrame(170'000,
        {pointingWithRatio(0.20)}, 5)).events.count == 0,
            "held pinch produces no repeated begin");
    require(pipeline.process(makeFrame(180'000,
        {pointingWithRatio(0.40)}, 6)).events.count == 0,
            "pinch release is debounced");
    auto end = pipeline.process(
        makeFrame(255'000, {pointingWithRatio(0.40)}, 7));
    require(end.events.count == 1 &&
                end.events.events[0].type == GestureEventType::PINCH_END,
            "pipeline emits one PINCH_END");
    require(end.observations[0].pose == StaticGesture::POINTING,
            "pipeline returns to POINTING after pinch");

    pipeline.reset();
    auto unknownHand = makeOpenHand(Handedness::UNKNOWN);
    auto unknown = pipeline.process(makeFrame(300'000, {unknownHand}, 8));
    require(unknown.observationCount == 0 && unknown.events.count == 0,
            "UNKNOWN hand does not contaminate nominal FSMs");

    pipeline.reset();
    auto both = pipeline.process(makeFrame(
        400'000,
        {pointingWithRatio(0.60, Handedness::RIGHT),
         pointingWithRatio(0.60, Handedness::LEFT)}, 9));
    require(both.observationCount == 2 && both.events.count == 2,
            "LEFT and RIGHT observations have independent FSMs");
    require(both.events.events[0].handedness == Handedness::LEFT &&
                both.events.events[1].handedness == Handedness::RIGHT,
            "multi-hand event order is deterministic LEFT then RIGHT");
    std::cout << "[PASS] test_gesture_pipeline\n";
    return 0;
}
