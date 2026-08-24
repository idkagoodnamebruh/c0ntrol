#include <cstdlib>
#include <iostream>

#include "src/core/actions/ActionDispatcher.h"
#include "src/core/actions/RecordingSystemInputBackend.h"
#include "src/core/config/RuntimeConfigController.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

InputConfig enabledInput(Handedness preferred = Handedness::RIGHT) {
    return {true, preferred};
}

GestureObservation observation(std::int64_t timestampUs, Handedness hand,
                               bool pointerActive, double x = 0.5,
                               double y = 0.5) {
    GestureObservation value;
    value.valid = true;
    value.pointerActive = pointerActive;
    value.handedness = hand;
    value.pointerPoint = {x, y, 0.0};
    value.timestampUs = timestampUs;
    value.frameId = static_cast<std::uint64_t>(timestampUs + 1);
    return value;
}

GestureEvent event(std::int64_t timestampUs, Handedness hand,
                   GestureEventType type) {
    return {type, hand, static_cast<std::uint64_t>(timestampUs + 1),
            timestampUs, {0.5, 0.5, 0.0}};
}

GesturePipelineResult frame(const GestureObservation& value) {
    GesturePipelineResult result;
    result.observations[result.observationCount++] = value;
    return result;
}

GesturePipelineResult frameWithEvent(const GestureObservation& value,
                                     GestureEventType type) {
    GesturePipelineResult result = frame(value);
    result.events.push(event(value.timestampUs, value.handedness, type));
    return result;
}

GesturePipelineResult eventOnly(std::int64_t timestampUs, Handedness hand,
                                GestureEventType type) {
    GesturePipelineResult result;
    result.events.push(event(timestampUs, hand, type));
    return result;
}

void testPointerPolicy() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    require(dispatcher.process(frame(observation(0, Handedness::RIGHT, false)))
                .commandCount == 0,
            "inactive pointer sends no move");
    dispatcher.process(frame(observation(10, Handedness::RIGHT, true, 0.2)));
    dispatcher.process(frame(observation(20, Handedness::RIGHT, true, 0.3)));
    require(backend.records.size() == 2 &&
                backend.records[0].type == RecordedInputType::MOVE &&
                backend.records[1].type == RecordedInputType::MOVE,
            "active pointer moves every valid frame");
}

void testPinchEdgesAndSuppression() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(frameWithEvent(
        observation(200, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(frame(observation(300, Handedness::RIGHT, true)));
    require(backend.buttonDownCount == 1,
            "BEGIN and held pinch produce exactly one DOWN");
    dispatcher.process(frameWithEvent(
        observation(400, Handedness::RIGHT, true),
        GestureEventType::PINCH_END));
    dispatcher.process(frameWithEvent(
        observation(500, Handedness::RIGHT, true),
        GestureEventType::PINCH_END));
    require(backend.buttonUpCount == 1,
            "END and repeated END produce exactly one UP");
}

void testCancel() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::LEFT, true),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(eventOnly(200, Handedness::LEFT,
                                 GestureEventType::PINCH_CANCEL));
    require(backend.buttonDownCount == 1 && backend.buttonUpCount == 1 &&
                !dispatcher.buttonDown(),
            "PINCH_CANCEL safely releases once");
}

void testShutdownAndDisableRelease() {
    RecordingSystemInputBackend shutdownBackend;
    ActionDispatcher shutdownDispatcher(shutdownBackend, {}, enabledInput());
    require(shutdownDispatcher.initialize(), "dispatcher initializes");
    shutdownDispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    shutdownDispatcher.shutdown();
    require(shutdownBackend.buttonUpCount == 1 &&
                shutdownBackend.shutdownCalled,
            "shutdown releases before backend shutdown");

    RecordingSystemInputBackend disableBackend;
    ActionDispatcher disableDispatcher(disableBackend, {}, enabledInput());
    require(disableDispatcher.initialize(), "dispatcher initializes");
    disableDispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    require(disableDispatcher.setInputEnabled(false) &&
                disableBackend.buttonUpCount == 1 &&
                !disableDispatcher.inputEnabled(),
            "disable releases held button");
    require(disableDispatcher.setInputEnabled(true),
            "re-enable starts from clean state");
}

void testActiveHandSwitch() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(frame(observation(200, Handedness::LEFT, true, 0.2)));
    require(backend.records.size() == 4 &&
                backend.records[0].type == RecordedInputType::BUTTON_DOWN &&
                backend.records[1].type == RecordedInputType::MOVE &&
                backend.records[2].type == RecordedInputType::BUTTON_UP &&
                backend.records[3].type == RecordedInputType::MOVE,
            "old hand releases before new hand moves");
    require(dispatcher.activeHand() == Handedness::LEFT,
            "LEFT takes control only after RIGHT release");
}

void testRightHandPreference() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    GesturePipelineResult both;
    both.observations[both.observationCount++] =
        observation(100, Handedness::LEFT, true, 0.1);
    both.observations[both.observationCount++] =
        observation(100, Handedness::RIGHT, true, 0.9);
    dispatcher.process(both);
    require(dispatcher.activeHand() == Handedness::RIGHT &&
                backend.records.size() == 1 &&
                backend.records[0].point.x > 1500,
            "RIGHT is preferred and hands never drive simultaneously");
}

void testConfiguredLeftHandPreference() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput(Handedness::LEFT));
    require(dispatcher.initialize(), "dispatcher initializes");
    GesturePipelineResult both;
    both.observations[both.observationCount++] =
        observation(100, Handedness::LEFT, true, 0.1);
    both.observations[both.observationCount++] =
        observation(100, Handedness::RIGHT, true, 0.9);
    dispatcher.process(both);
    require(dispatcher.activeHand() == Handedness::LEFT &&
                backend.records.size() == 1 &&
                backend.records[0].point.x < 500,
            "persisted LEFT preference selects LEFT without dual control");
}

void testTimestampReplay() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    const std::size_t before = backend.records.size();
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(frameWithEvent(
        observation(99, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    require(backend.records.size() == before && backend.buttonDownCount == 1,
            "duplicate/regressive timestamps produce no input");
}

void testDragSemanticOrder() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    dispatcher.process(frame(observation(100, Handedness::RIGHT, true, 0.1)));
    dispatcher.process(frameWithEvent(
        observation(200, Handedness::RIGHT, true, 0.2),
        GestureEventType::PINCH_BEGIN));
    dispatcher.process(frame(observation(300, Handedness::RIGHT, true, 0.3)));
    dispatcher.process(eventOnly(400, Handedness::RIGHT,
                                 GestureEventType::PINCH_END));
    const RecordedInputType expected[] = {
        RecordedInputType::MOVE,
        RecordedInputType::BUTTON_DOWN,
        RecordedInputType::MOVE,
        RecordedInputType::MOVE,
        RecordedInputType::BUTTON_UP,
    };
    require(backend.records.size() == 5, "drag emits five exact actions");
    for (std::size_t i = 0; i < 5; ++i)
        require(backend.records[i].type == expected[i],
                "drag semantic order is MOVE DOWN MOVE MOVE UP");
}

void testFailureRecovery() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "dispatcher initializes");
    backend.failNextMove = true;
    const auto result = dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    require(!result.success && backend.buttonDownCount == 1 &&
                backend.buttonUpCount == 1 && !dispatcher.buttonDown(),
            "move failure after DOWN attempts one recovery UP");

    RecordingSystemInputBackend releaseBackend;
    ActionDispatcher releaseDispatcher(releaseBackend, {}, enabledInput());
    require(releaseDispatcher.initialize(), "dispatcher initializes");
    releaseDispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    releaseBackend.failNextUp = true;
    const auto failedEnd = releaseDispatcher.process(frameWithEvent(
        observation(200, Handedness::RIGHT, true),
        GestureEventType::PINCH_END));
    require(!failedEnd.success && releaseDispatcher.buttonDown(),
            "failed UP preserves ownership for later recovery");
    releaseDispatcher.shutdown();
    require(releaseBackend.buttonUpCount == 1 && releaseBackend.shutdownCalled,
            "shutdown retries UP once before backend shutdown");
}

void testSafeDefaultAndRuntimeConfigSafety() {
    RecordingSystemInputBackend disabledBackend;
    ActionDispatcher disabled(disabledBackend);
    require(disabled.initialize() && !disabled.inputEnabled(),
            "first-run ActionDispatcher is safely disabled");
    disabled.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    require(disabledBackend.records.empty(),
            "disabled input emits no native commands");
    require(disabled.setInputEnabled(true) && !disabled.buttonDown(),
            "explicit enable starts from a clean state");
    disabled.process(frame(observation(200, Handedness::RIGHT, true)));
    require(disabledBackend.records.size() == 1 &&
                disabledBackend.records[0].type == RecordedInputType::MOVE,
            "explicit enable permits input");

    RecordingSystemInputBackend backend;
    RuntimeConfig initial;
    initial.input = enabledInput();
    ActionDispatcher dispatcher(backend, initial.pointer, initial.input);
    require(dispatcher.initialize(), "configured dispatcher initializes");
    RuntimeConfigController controller(initial, dispatcher);
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    RuntimeConfig cameraChanged = initial;
    cameraChanged.camera.index = 2;
    const auto restart = controller.apply(cameraChanged);
    require(restart.success && restart.changes.cameraRestartRequired &&
                backend.buttonUpCount == 1 && !dispatcher.buttonDown(),
            "camera/config restart releases held button exactly once");

    dispatcher.process(frameWithEvent(
        observation(200, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    const auto reset = controller.resetToDefaults();
    require(reset.success && backend.buttonUpCount == 2 &&
                !dispatcher.buttonDown() && !dispatcher.inputEnabled() &&
                controller.current() == RuntimeConfig{},
            "settings reset releases once and restores safe defaults");
}

} // namespace

int main() {
    testPointerPolicy();
    testPinchEdgesAndSuppression();
    testCancel();
    testShutdownAndDisableRelease();
    testActiveHandSwitch();
    testRightHandPreference();
    testConfiguredLeftHandPreference();
    testTimestampReplay();
    testDragSemanticOrder();
    testFailureRecovery();
    testSafeDefaultAndRuntimeConfigSafety();
    std::cout << "[PASS] test_action_dispatcher (safe config + recovery)\n";
    return 0;
}
