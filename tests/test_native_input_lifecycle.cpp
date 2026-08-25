#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "src/core/input/NativeInputRuntime.h"
#include "src/platform/NullSystemInputBackend.h"
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

void testDisabledStartupAndSuccessfulActivation() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));

    require(runtime.status().state == NativeInputState::DISABLED &&
                !runtime.status().desiredEnabled &&
                state->snapshot().initializeCount == 0,
            "disabled startup never initializes the backend");

    runtime.requestEnabled(true);
    require(runtime.status().state == NativeInputState::ACTIVATING &&
                runtime.status().desiredEnabled,
            "enable returns to its caller with ACTIVATING visible");
    require(state->waitForInitializeCount(1, kDeadline),
            "worker begins the blocking initialize attempt");
    require(runtime.status().state == NativeInputState::ACTIVATING,
            "state remains ACTIVATING while initialize is blocked");

    state->completeInitialize(1, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "successful initialize publishes READY");
    runtime.shutdown();
    require(state->snapshot().ownerThreadViolations == 0,
            "backend initialize, geometry and shutdown share one owner thread");
}

void testFailureErrorAndRetry() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));

    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "failed attempt starts");
    state->completeInitialize(1, false);
    require(runtime.waitForState(NativeInputState::FAILED, kDeadline),
            "failed initialize reaches FAILED");
    const NativeInputStatus failed = runtime.status();
    require(!failed.desiredEnabled &&
                failed.error == "fake blocking initialization failed",
            "FAILED preserves the real backend error and effective off state");

    runtime.requestEnabled(true);
    require(runtime.status().state == NativeInputState::ACTIVATING,
            "explicit retry leaves FAILED for ACTIVATING");
    require(state->waitForInitializeCount(2, kDeadline),
            "retry performs a new backend initialize");
    state->completeInitialize(2, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "retry can reach READY");
    runtime.shutdown();
}

void testDisableDuringActivationIgnoresLateSuccess() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));

    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "cancelled activation starts");
    runtime.requestEnabled(false);
    require(runtime.status().state == NativeInputState::STOPPING &&
                !runtime.status().desiredEnabled,
            "disable is recorded immediately while initialize is blocked");
    state->completeInitialize(1, true);
    require(runtime.waitForState(NativeInputState::DISABLED, kDeadline),
            "late success is discarded into DISABLED");

    const auto snapshot = state->snapshot();
    require(snapshot.shutdownCount == 1 && snapshot.records.empty(),
            "cancelled successful backend is shut down without native events");
}

void testStaleGenerationCannotPublishReady() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));

    runtime.requestEnabled(true);             // generation 1
    require(state->waitForInitializeCount(1, kDeadline),
            "generation one starts");
    runtime.requestEnabled(false);            // generation 2
    runtime.requestEnabled(true);             // generation 3
    const std::uint64_t newestGeneration = runtime.status().generation;
    state->completeInitialize(1, true);
    require(state->waitForInitializeCount(2, kDeadline),
            "stale success is torn down before newest activation starts");
    require(runtime.status().state == NativeInputState::ACTIVATING &&
                runtime.status().generation == newestGeneration,
            "stale generation cannot adopt READY");
    state->completeInitialize(2, true);
    require(runtime.waitForState(NativeInputState::READY, kDeadline),
            "newest generation alone adopts READY");
    require(runtime.status().generation == newestGeneration &&
                state->snapshot().shutdownCount == 1,
            "stale initialized session was shut down exactly once");
    runtime.shutdown();
}

void testShutdownDuringActivationNeverPublishesReady() {
    auto state = std::make_shared<FakeBlockingInputState>();
    NativeInputRuntime runtime(
        std::make_unique<FakeBlockingSystemInputBackend>(state));
    runtime.requestEnabled(true);
    require(state->waitForInitializeCount(1, kDeadline),
            "shutdown activation starts");

    std::thread shutdownCaller([&runtime] { runtime.shutdown(); });
    require(runtime.waitForState(NativeInputState::STOPPING, kDeadline),
            "shutdown records STOPPING without waiting for initialize");
    state->completeInitialize(1, true);
    shutdownCaller.join();
    require(runtime.status().state == NativeInputState::DISABLED &&
                !runtime.status().desiredEnabled &&
                state->snapshot().records.empty(),
            "shutdown joins cleanly and a late success never publishes input");
}

void testNullBackendFailsFinitelyAndRetriesSafely() {
    NativeInputRuntime runtime(std::make_unique<NullSystemInputBackend>());
    runtime.requestEnabled(true);
    require(runtime.waitForState(NativeInputState::FAILED, kDeadline),
            "Null backend reaches FAILED instead of permanent ACTIVATING");
    const std::uint64_t failedGeneration = runtime.status().generation;
    require(runtime.status().error.find("unsupported") != std::string::npos,
            "Null backend exposes its stable unsupported error");
    runtime.requestEnabled(true);
    require(runtime.waitForState(NativeInputState::FAILED, kDeadline) &&
                runtime.status().generation == failedGeneration + 1,
            "Null backend retry is finite and safe");
}

} // namespace

int main() {
    testDisabledStartupAndSuccessfulActivation();
    testFailureErrorAndRetry();
    testDisableDuringActivationIgnoresLateSuccess();
    testStaleGenerationCannotPublishReady();
    testShutdownDuringActivationNeverPublishesReady();
    testNullBackendFailsFinitelyAndRetriesSafely();
    std::cout << "[PASS] test_native_input_lifecycle\n";
    return 0;
}
