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

TrackedHand positioned(TrackedHand hand, double x, double y) {
    return transformed(std::move(hand), 1.0, 0.0, {x, y, 0.0});
}

bool hasEvent(const GesturePipelineResult& result, GestureEventType type,
              Handedness hand = Handedness::RIGHT) {
    for (std::size_t i = 0; i < result.events.count; ++i) {
        if (result.events.events[i].type == type &&
            result.events.events[i].handedness == hand) {
            return true;
        }
    }
    return false;
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

    GesturePipeline dynamicPipeline;
    auto openStationary = dynamicPipeline.process(makeFrame(
        1'000'000, {positioned(makeOpenHand(), 0.5, 0.75)}, 20));
    require(!hasEvent(openStationary, GestureEventType::SWIPE_UP),
            "stationary OPEN_HAND starts dynamic history without an event");
    auto openMoving = dynamicPipeline.process(makeFrame(
        1'050'000, {positioned(makeOpenHand(), 0.5, 0.58)}, 21));
    require(!hasEvent(openMoving, GestureEventType::SWIPE_UP),
            "intermediate OPEN_HAND movement remains a candidate");
    auto swipeUp = dynamicPipeline.process(makeFrame(
        1'100'000, {positioned(makeOpenHand(), 0.5, 0.30)}, 22));
    require(swipeUp.events.count == 1 &&
                hasEvent(swipeUp, GestureEventType::SWIPE_UP) &&
                swipeUp.events.events[0].frameId == 22 &&
                swipeUp.events.events[0].timestampUs == 1'100'000,
            "pipeline emits exact timestamped SWIPE_UP once");
    auto cooldown = dynamicPipeline.process(makeFrame(
        1'200'000, {positioned(makeOpenHand(), 0.5, 0.10)}, 23));
    require(!hasEvent(cooldown, GestureEventType::SWIPE_UP),
            "pipeline cooldown suppresses duplicate swipe");
    dynamicPipeline.process(makeFrame(
        1'510'000, {positioned(makeOpenHand(), 0.5, 0.30)}, 24));
    dynamicPipeline.process(makeFrame(
        1'560'000, {positioned(makeOpenHand(), 0.5, 0.48)}, 25));
    auto swipeDown = dynamicPipeline.process(makeFrame(
        1'610'000, {positioned(makeOpenHand(), 0.5, 0.75)}, 26));
    require(hasEvent(swipeDown, GestureEventType::SWIPE_DOWN) &&
                swipeDown.events.events[0].frameId == 26 &&
                swipeDown.events.events[0].timestampUs == 1'610'000,
            "pipeline emits a new SWIPE_DOWN after cooldown");

    dynamicPipeline.reset();
    auto pointingStart = dynamicPipeline.process(makeFrame(
        2'000'000, {positioned(makePointingHand(), 0.3, 0.5)}, 30));
    dynamicPipeline.process(makeFrame(
        2'050'000, {positioned(makePointingHand(), 0.5, 0.5)}, 31));
    auto pointingEnd = dynamicPipeline.process(makeFrame(
        2'100'000, {positioned(makePointingHand(), 0.8, 0.5)}, 32));
    require(hasEvent(pointingStart, GestureEventType::POINTER_ACTIVE) &&
                !hasEvent(pointingStart, GestureEventType::SWIPE_RIGHT) &&
                !hasEvent(pointingEnd, GestureEventType::SWIPE_RIGHT),
            "POINTING movement emits pointer state but never a swipe");

    dynamicPipeline.reset();
    dynamicPipeline.process(makeFrame(
        3'000'000, {positioned(makeOpenHand(), 0.3, 0.5)}, 40));
    dynamicPipeline.process(makeFrame(
        3'050'000, {positioned(makeOpenHand(), 0.5, 0.5)}, 41));
    auto duplicateRight = positioned(makeOpenHand(), 0.7, 0.5);
    auto ambiguous = dynamicPipeline.process(makeFrame(
        3'100'000, {duplicateRight, duplicateRight}, 42));
    auto afterAmbiguous = dynamicPipeline.process(makeFrame(
        3'150'000, {positioned(makeOpenHand(), 0.8, 0.5)}, 43));
    require(ambiguous.observationCount == 0 &&
                !hasEvent(ambiguous, GestureEventType::SWIPE_RIGHT) &&
                !hasEvent(afterAmbiguous, GestureEventType::SWIPE_RIGHT),
            "duplicate handedness resets history without cross-contamination");

    GestureEventBuffer<2> bounded;
    require(bounded.push({}) && bounded.push({}) && !bounded.push({}) &&
                bounded.count == 2 && bounded.droppedCount == 1,
            "event buffer exposes overflow instead of dropping silently");
    std::cout << "[PASS] test_gesture_pipeline\n";
    return 0;
}
