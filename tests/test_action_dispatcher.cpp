#include <cstdlib>
#include <iostream>

#include "src/core/actions/ActionDispatcher.h"
#include "src/core/actions/RecordingSystemInputBackend.h"
#include "src/core/config/RuntimeConfigController.h"
#include "src/platform/NullSystemInputBackend.h"

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

void testVerticalSwipeScrolling() {
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, {}, enabledInput());
    require(dispatcher.initialize(), "scroll dispatcher initializes");

    const auto up = dispatcher.process(eventOnly(
        100, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(up.success && up.commandCount == 1 &&
                up.commands[0].type == ActionType::SCROLL_VERTICAL &&
                up.commands[0].scrollNotches == 3 &&
                backend.scrollCount == 1 &&
                backend.records.back().scrollNotches == 3,
            "SWIPE_UP emits exactly one positive logical scroll");

    const auto down = dispatcher.process(eventOnly(
        200, Handedness::RIGHT, GestureEventType::SWIPE_DOWN));
    require(down.success && down.commandCount == 1 &&
                down.commands[0].scrollNotches == -3 &&
                backend.scrollCount == 2,
            "SWIPE_DOWN emits the opposite scroll sign");
    dispatcher.process(eventOnly(
        200, Handedness::RIGHT, GestureEventType::SWIPE_DOWN));
    require(backend.scrollCount == 2,
            "replayed swipe timestamp cannot duplicate scroll");

    GesturePipelineResult both;
    both.events.push(event(300, Handedness::LEFT,
                           GestureEventType::SWIPE_DOWN));
    both.events.push(event(300, Handedness::RIGHT,
                           GestureEventType::SWIPE_UP));
    const auto preferred = dispatcher.process(both);
    require(preferred.commandCount == 1 && backend.scrollCount == 3 &&
                preferred.commands[0].handedness == Handedness::RIGHT &&
                preferred.commands[0].scrollNotches == 3,
            "preferred hand limits simultaneous swipes to one scroll");

    dispatcher.process(eventOnly(
        400, Handedness::LEFT, GestureEventType::SWIPE_UP));
    require(backend.scrollCount == 4,
            "single non-preferred hand is an allowed fallback");
    dispatcher.process(eventOnly(
        500, Handedness::RIGHT, GestureEventType::SWIPE_LEFT));
    require(backend.scrollCount == 4,
            "horizontal swipe has no OS action");
}

void testScrollSafetyAndConfiguration() {
    RecordingSystemInputBackend dragBackend;
    ActionDispatcher drag(dragBackend, {}, enabledInput());
    require(drag.initialize(), "drag dispatcher initializes");
    drag.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    require(drag.buttonDown(), "test owns primary button before swipe");
    drag.process(eventOnly(
        200, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(dragBackend.scrollCount == 0 && dragBackend.buttonUpCount == 1,
            "scroll is suppressed when the frame began with button down");

    RecordingSystemInputBackend disabledBackend;
    ActionDispatcher disabled(disabledBackend);
    require(disabled.initialize(), "disabled dispatcher initializes");
    disabled.process(eventOnly(
        100, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(disabledBackend.scrollCount == 0,
            "master input disabled blocks native scroll");

    InputConfig scrollOff = enabledInput();
    scrollOff.swipeScrollEnabled = false;
    RecordingSystemInputBackend scrollOffBackend;
    ActionDispatcher scrollDisabled(scrollOffBackend, {}, scrollOff);
    require(scrollDisabled.initialize(), "scroll-disabled dispatcher initializes");
    scrollDisabled.process(eventOnly(
        100, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(scrollOffBackend.scrollCount == 0,
            "swipe-scroll setting blocks scroll without removing events");

    InputConfig inverted = enabledInput();
    inverted.scrollNotchesPerSwipe = 5;
    inverted.invertSwipeScroll = true;
    RecordingSystemInputBackend invertedBackend;
    ActionDispatcher invertedDispatcher(invertedBackend, {}, inverted);
    require(invertedDispatcher.initialize(), "inverted dispatcher initializes");
    const auto invertedResult = invertedDispatcher.process(eventOnly(
        100, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(invertedResult.commandCount == 1 &&
                invertedResult.commands[0].scrollNotches == -5,
            "invert setting reverses configured scroll amount");

    RecordingSystemInputBackend failedBackend;
    ActionDispatcher failed(failedBackend, {}, enabledInput());
    require(failed.initialize(), "failure dispatcher initializes");
    failedBackend.failNextScroll = true;
    const auto failure = failed.process(eventOnly(
        100, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(!failure.success && !failure.error.empty() &&
                failedBackend.scrollCount == 0,
            "backend scroll failure propagates without a fake command");
}

void testSettingsModalInputSuspension() {
    RuntimeConfig initial;
    initial.input = enabledInput();
    RecordingSystemInputBackend backend;
    ActionDispatcher dispatcher(backend, initial.pointer, initial.input);
    require(dispatcher.initialize(), "modal dispatcher initializes");
    RuntimeConfigController controller(initial, dispatcher);
    dispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));

    std::string error;
    require(controller.suspendInput(error) && error.empty() &&
                controller.inputSuspended() &&
                !dispatcher.inputEnabled() && !dispatcher.buttonDown() &&
                backend.buttonUpCount == 1 &&
                controller.current().input.enabled,
            "opening settings suspends input and releases without persisting");
    dispatcher.process(eventOnly(
        200, Handedness::RIGHT, GestureEventType::SWIPE_UP));
    require(backend.scrollCount == 0,
            "settings/calibration suspension blocks MOVE/DOWN/UP/SCROLL");
    require(controller.cancelInputSuspension(error) &&
                dispatcher.inputEnabled() && !controller.inputSuspended() &&
                controller.current() == initial,
            "Cancel restores prior input state without changing settings");

    require(controller.suspendInput(error),
            "second settings session suspends input");
    RuntimeConfig requested = initial;
    requested.input.scrollNotchesPerSwipe = 6;
    requested.input.invertSwipeScroll = true;
    const auto saved = controller.completeInputSuspension(requested, false);
    require(saved.success && !controller.inputSuspended() &&
                dispatcher.inputEnabled() &&
                dispatcher.inputConfig().scrollNotchesPerSwipe == 6 &&
                dispatcher.inputConfig().invertSwipeScroll &&
                controller.current() == sanitizeRuntimeConfig(requested),
            "Save applies the requested input configuration");

    RecordingSystemInputBackend failedBackend;
    ActionDispatcher failedDispatcher(failedBackend, initial.pointer,
                                      initial.input);
    require(failedDispatcher.initialize(), "failed modal dispatcher initializes");
    RuntimeConfigController failedController(initial, failedDispatcher);
    failedDispatcher.process(frameWithEvent(
        observation(100, Handedness::RIGHT, true),
        GestureEventType::PINCH_BEGIN));
    failedBackend.failNextUp = true;
    require(!failedController.suspendInput(error) && !error.empty() &&
                !failedController.inputSuspended() &&
                !failedDispatcher.inputEnabled() &&
                failedDispatcher.buttonDown(),
            "failed release is reported and cannot silently open settings");
    require(failedController.suspendInput(error) &&
                failedController.inputSuspended() &&
                !failedDispatcher.buttonDown() &&
                failedBackend.buttonUpCount == 1,
            "a later settings attempt retries the failed safety release");
    require(failedController.cancelInputSuspension(error) &&
                failedDispatcher.inputEnabled(),
            "successful retry still restores the pre-dialog enabled state");
}

void testUnsupportedScrollIsExplicit() {
    NullSystemInputBackend backend;
    require(!backend.scrollVertical(1) &&
                backend.lastError().find("unsupported") != std::string::npos,
            "unsupported backend reports scroll failure explicitly");
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
    testVerticalSwipeScrolling();
    testScrollSafetyAndConfiguration();
    testSettingsModalInputSuspension();
    testUnsupportedScrollIsExplicit();
    std::cout << "[PASS] test_action_dispatcher (safe config + recovery)\n";
    return 0;
}
