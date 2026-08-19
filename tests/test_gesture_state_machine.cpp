#include <iostream>

#include "tests/GestureTestFixtures.h"
#include "src/core/gestures/GestureStateMachine.h"

using namespace gesture_test;

namespace {
GestureObservation observation(std::int64_t time, double ratio,
                               bool pointer = false,
                               Handedness hand = Handedness::RIGHT,
                               bool valid = true) {
    GestureObservation result;
    result.valid = valid;
    result.handedness = hand;
    result.timestampUs = time;
    result.frameId = static_cast<std::uint64_t>(time + 1);
    result.pinchRatio = ratio;
    result.pointerActive = pointer;
    result.pointerPoint = {0.4, 0.3, 0.0};
    return result;
}

void requireEvent(const GestureEventBatch& events, GestureEventType type,
                  const char* message) {
    require(events.count == 1 && events.events[0].type == type, message);
}

void testNoiseAndSustainedEntry() {
    GestureStateMachine idle(Handedness::RIGHT);
    require(idle.update(observation(0, 1.0)).count == 0 &&
                idle.pinchState() == PinchState::IDLE,
            "idle remains stable without pinch input");

    GestureStateMachine fsm(Handedness::RIGHT);
    require(fsm.update(observation(0, 0.24)).count == 0,
            "entry starts as a candidate");
    require(fsm.update(observation(40'000, 0.27)).count == 0,
            "noise above enter threshold does not begin pinch");
    require(fsm.pinchState() == PinchState::IDLE,
            "failed entry candidate returns idle");
    fsm.update(observation(50'000, 0.20));
    requireEvent(fsm.update(observation(125'000, 0.20)),
                 GestureEventType::PINCH_BEGIN,
                 "sustained entry emits one PINCH_BEGIN");
    require(fsm.update(observation(1'125'000, 0.20)).count == 0,
            "one-second pinch never repeats PINCH_BEGIN");
}

void testReleaseHysteresisAndRepinch() {
    GestureStateMachine fsm(Handedness::RIGHT);
    fsm.update(observation(0, 0.20));
    fsm.update(observation(75'000, 0.20));
    require(fsm.update(observation(80'000, 0.36)).count == 0,
            "release starts as a candidate");
    require(fsm.update(observation(120'000, 0.34)).count == 0,
            "release noise inside hysteresis does not end pinch");
    require(fsm.pinchState() == PinchState::PINCHED,
            "release noise returns to pinched");
    fsm.update(observation(130'000, 0.40));
    requireEvent(fsm.update(observation(205'000, 0.40)),
                 GestureEventType::PINCH_END,
                 "sustained release emits one PINCH_END");
    require(fsm.update(observation(220'000, 0.40)).count == 0,
            "released state does not repeat PINCH_END");
    fsm.update(observation(230'000, 0.20));
    requireEvent(fsm.update(observation(305'000, 0.20)),
                 GestureEventType::PINCH_BEGIN,
                 "a later pinch can begin again");
}

void testTrackingLoss() {
    GestureStateMachine shortLoss(Handedness::RIGHT);
    shortLoss.update(observation(0, 0.20));
    shortLoss.update(observation(75'000, 0.20));
    require(shortLoss.update(observation(100'000, 1.0, false,
                                         Handedness::RIGHT, false)).count == 0,
            "short loss does not finish active pinch");
    require(shortLoss.update(observation(200'000, 0.20)).count == 0 &&
                shortLoss.pinchState() == PinchState::PINCHED,
            "tracking recovery before timeout preserves pinch");

    GestureStateMachine longLoss(Handedness::RIGHT);
    longLoss.update(observation(0, 0.20));
    longLoss.update(observation(75'000, 0.20));
    longLoss.update(observation(100'000, 1.0, false,
                                Handedness::RIGHT, false));
    requireEvent(longLoss.update(observation(250'000, 1.0, false,
                                             Handedness::RIGHT, false)),
                 GestureEventType::PINCH_CANCEL,
                 "long tracking loss emits one PINCH_CANCEL");
    require(longLoss.update(observation(300'000, 1.0, false,
                                        Handedness::RIGHT, false)).count == 0,
            "tracking loss does not repeat cancellation");
}

void testPointerAndTimestampPolicy() {
    GestureStateMachine fsm(Handedness::RIGHT);
    requireEvent(fsm.update(observation(100, 1.0, true)),
                 GestureEventType::POINTER_ACTIVE,
                 "pointer activation is edge-triggered");
    require(fsm.update(observation(100, 1.0, false)).count == 0,
            "duplicate timestamp is ignored");
    require(fsm.update(observation(99, 1.0, false)).count == 0,
            "regressive timestamp is ignored");
    require(fsm.pointerActive(), "ignored timestamps do not mutate state");
    requireEvent(fsm.update(observation(101, 1.0, false)),
                 GestureEventType::POINTER_INACTIVE,
                 "pointer deactivation is edge-triggered");
    require(fsm.update(observation(102, 1.0, false,
                                   Handedness::LEFT)).count == 0,
            "mismatched handedness cannot activate this FSM");
}
} // namespace

int main() {
    testNoiseAndSustainedEntry();
    testReleaseHysteresisAndRepinch();
    testTrackingLoss();
    testPointerAndTimestampPolicy();
    std::cout << "[PASS] test_gesture_state_machine (11 cases)\n";
    return 0;
}
