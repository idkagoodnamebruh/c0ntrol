#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>

#include "src/core/input/NativeInputRuntime.h"
#include "tests/support/FakeBlockingSystemInputBackend.h"

namespace {

using namespace std::chrono_literals;
constexpr auto kDeadline = 2s;

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

InputConfig enabledInput() {
    InputConfig config;
    config.enabled = true;
    return config;
}

GestureObservation observation(std::int64_t timestampUs, double x = 0.5,
                               Handedness hand = Handedness::RIGHT) {
    GestureObservation value;
    value.valid = true;
    value.pointerActive = true;
    value.handedness = hand;
    value.pointerPoint = {x, 0.5, 0.0};
    value.frameId = static_cast<std::uint64_t>(timestampUs + 1);
    value.timestampUs = timestampUs;
    return value;
}

GestureEvent event(std::int64_t timestampUs, GestureEventType type,
                   Handedness hand = Handedness::RIGHT) {
    return {type, hand, static_cast<std::uint64_t>(timestampUs + 1),
            timestampUs, {0.5, 0.5, 0.0}};
}

GesturePipelineResult pointerFrame(std::int64_t timestampUs,
                                   double x = 0.5) {
    GesturePipelineResult result;
    result.observations[result.observationCount++] =
        observation(timestampUs, x);
    return result;
}

GesturePipelineResult semanticFrame(std::int64_t timestampUs,
                                    GestureEventType type,
                                    double x = 0.5) {
    GesturePipelineResult result = pointerFrame(timestampUs, x);
    result.events.push(event(timestampUs, type));
    return result;
}

void activate(NativeInputRuntime& runtime,
              const std::shared_ptr<FakeBlockingInputState>& state,
              int attempt = 1) {
    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(attempt, kDeadline),
            "activation reaches fake backend");
    state->completeInitialize(attempt, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "activation reaches READY");
}

void testNoInputOrReplayBeforeReady() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));

    runtime.submitLatest(semanticFrame(10, GestureEventType::PINCH_BEGIN));
    require(state->snapshot().records.empty(),
            "DISABLED drops every native-input candidate");

    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "pre-READY activation starts");
    runtime.submitLatest(pointerFrame(100, 0.1));
    runtime.submitLatest(semanticFrame(101, GestureEventType::PINCH_BEGIN));
    runtime.submitLatest(semanticFrame(102, GestureEventType::SWIPE_UP));
    require(state->snapshot().records.empty(),
            "ACTIVATING emits no move, button or scroll");

    state->completeInitialize(1, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "pre-READY test reaches READY");
    require(state->snapshot().records.empty(),
            "activation boundary replays no pending gesture");

    runtime.submitLatest(pointerFrame(200, 0.25));
    require(state->waitForMoveCount(1, kDeadline),
            "first post-READY pointer frame is processed");
    const auto snapshot = state->snapshot();
    require(snapshot.records.size() == 1 &&
                snapshot.records[0].type == RecordedInputType::MOVE &&
                snapshot.records[0].point.x == 250,
            "first native command derives from the post-READY sample");
    runtime.shutdown();
}

void testFailedAndDisabledStatesRejectEvents() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));
    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "failure rejection activation starts");
    state->completeInitialize(1, false);
    require(runtime.waitForState(NativeInputState::FAILED, kDeadline),
            "failure rejection reaches FAILED");
    runtime.submitLatest(semanticFrame(100, GestureEventType::PINCH_BEGIN));
    runtime.submitLatest(semanticFrame(101, GestureEventType::SWIPE_UP));
    require(state->snapshot().records.empty(),
            "FAILED emits no native actions");
    runtime.requestEnabled(false);
    require(runtime.waitForState(NativeInputState::DISABLED, kDeadline),
            "explicit disable leaves FAILED for DISABLED");
    runtime.submitLatest(pointerFrame(102));
    require(state->snapshot().records.empty(),
            "DISABLED remains non-dispatching after a failure");
}

void testNewestConfigurationWinsActivationRace() {
    auto state = std::make_shared<FakeBlockingInputState>();
    PointerMappingConfig original;
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state), original);
    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "configuration race activation starts");

    PointerMappingConfig newest;
    newest.mirrorX = true;
    runtime.requestConfiguration(newest, enabledInput());
    state->completeInitialize(1, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "configuration race reaches READY");
    runtime.submitLatest(pointerFrame(100, 0.2));
    require(state->waitForMoveCount(1, kDeadline),
            "newest mapping produces a move");
    require(state->snapshot().records.back().point.x == 799,
            "READY applies config B instead of stale activation config A");
    runtime.shutdown();
}

void testDisableAndShutdownReleaseExactlyOnce() {
    auto disableState = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime disableRuntime(
        std::make_unique<FakeBlockingSystemInputBackend>(disableState));
    activate(disableRuntime, disableState);
    disableRuntime.submitLatest(
        semanticFrame(100, GestureEventType::PINCH_BEGIN));
    require(disableState->waitForButtonDownCount(1, kDeadline),
            "disable case owns the button");
    disableRuntime.requestEnabled(false);
    require(disableRuntime.waitForState(NativeInputState::DISABLED, kDeadline),
            "disable completes after release and shutdown");
    require(disableState->snapshot().buttonUpCount == 1,
            "button DOWN to disable emits exactly one UP");
    disableRuntime.requestEnabled(false);
    require(disableState->snapshot().buttonUpCount == 1,
            "double disable emits no duplicate UP");
    disableRuntime.submitLatest(pointerFrame(200));
    require(disableState->snapshot().moveCount == 1,
            "suspended settings/calibration interval rejects input");
    disableRuntime.shutdown();
    require(disableState->snapshot().buttonUpCount == 1,
            "shutdown after disable still emits no duplicate UP");

    auto shutdownState = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime shutdownRuntime(
        std::make_unique<FakeBlockingSystemInputBackend>(shutdownState));
    activate(shutdownRuntime, shutdownState);
    shutdownRuntime.submitLatest(
        semanticFrame(100, GestureEventType::PINCH_BEGIN));
    require(shutdownState->waitForButtonDownCount(1, kDeadline),
            "shutdown case owns the button");
    shutdownRuntime.shutdown();
    require(shutdownState->snapshot().buttonUpCount == 1,
            "button DOWN to shutdown emits exactly one UP");
}

void testTimestampProtectionsRemainIntact() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));
    activate(runtime, state);

    runtime.submitLatest(pointerFrame(100, 0.1));
    require(state->waitForMoveCount(1, kDeadline),
            "timestamp baseline move is processed");
    runtime.submitLatest(semanticFrame(100, GestureEventType::POINTER_ACTIVE,
                                       0.2));
    runtime.submitLatest(semanticFrame(99, GestureEventType::POINTER_ACTIVE,
                                       0.3));
    runtime.submitLatest(semanticFrame(101, GestureEventType::POINTER_ACTIVE,
                                       0.4));
    require(state->waitForMoveCount(2, kDeadline),
            "later accepted frame acts as deterministic drain barrier");
    require(state->snapshot().moveCount == 2,
            "duplicate and regressive frames emit no native moves");
    runtime.shutdown();
}

void testBoundedQueueAndReleasePreservation() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));
    activate(runtime, state);

    state->setMoveBlocked(true);
    runtime.submitLatest(pointerFrame(100));
    require(state->waitForMoveEnteredCount(1, kDeadline),
            "worker is deterministically held in one native move");
    for (int i = 0; i < 1000; ++i)
        runtime.submitLatest(pointerFrame(200 + i, 0.1));
    for (int i = 0; i < 100; ++i) {
        runtime.submitLatest(semanticFrame(
            2000 + i, GestureEventType::POINTER_ACTIVE, 0.2));
    }
    const NativeInputStatus pressure = runtime.status();
    require(pressure.pendingFrameCount <= 1 &&
                pressure.pendingSemanticFrameCount <=
                    NativeInputRuntime::kMaxPendingSemanticFrames &&
                pressure.droppedFrameCount > 0 &&
                pressure.droppedSemanticFrameCount > 0,
            "camera and semantic handoff remain explicitly bounded");
    state->setMoveBlocked(false);
    runtime.requestEnabled(false);
    require(runtime.waitForState(NativeInputState::DISABLED, kDeadline),
            "queue pressure can still be disabled cleanly");

    auto releaseState = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime releaseRuntime(
        std::make_unique<FakeBlockingSystemInputBackend>(releaseState));
    activate(releaseRuntime, releaseState);
    releaseRuntime.submitLatest(
        semanticFrame(100, GestureEventType::PINCH_BEGIN));
    require(releaseState->waitForButtonDownCount(1, kDeadline),
            "release-preservation case owns the button");
    require(releaseState->waitForMoveCount(1, kDeadline),
            "DOWN frame finishes before queue pressure begins");
    releaseState->setMoveBlocked(true);
    releaseRuntime.submitLatest(pointerFrame(200));
    require(releaseState->waitForMoveEnteredCount(2, kDeadline),
            "release-preservation worker is held after DOWN");
    for (int i = 0; i < 100; ++i) {
        releaseRuntime.submitLatest(semanticFrame(
            300 + i, GestureEventType::POINTER_ACTIVE, 0.3));
    }
    releaseRuntime.submitLatest(
        semanticFrame(500, GestureEventType::PINCH_END));
    require(releaseRuntime.status().pendingSemanticFrameCount <=
                NativeInputRuntime::kMaxPendingSemanticFrames,
            "release preservation never expands the semantic queue");
    releaseState->setMoveBlocked(false);
    require(releaseState->waitForButtonUpCount(1, kDeadline),
            "bounded pressure preserves the safety-critical PINCH_END");
    require(releaseState->snapshot().buttonUpCount == 1,
            "preserved semantic release emits exactly one UP");
    releaseRuntime.shutdown();
}

} // namespace

int main() {
    testNoInputOrReplayBeforeReady();
    testFailedAndDisabledStatesRejectEvents();
    testNewestConfigurationWinsActivationRace();
    testDisableAndShutdownReleaseExactlyOnce();
    testTimestampProtectionsRemainIntact();
    testBoundedQueueAndReleasePreservation();
    std::cout << "[PASS] test_async_input_runtime\n";
    return 0;
}
