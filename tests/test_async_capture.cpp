#include <chrono>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

#include "src/core/capture/AsyncCapture.h"
#include "tests/support/FakeCameraSource.h"

namespace {

using namespace std::chrono_literals;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

template <typename Predicate>
bool waitUntil(Predicate predicate,
               std::chrono::milliseconds timeout = 2s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate()) return true;
        std::this_thread::sleep_for(1ms);
    }
    return predicate();
}

} // namespace

int main() {
    {
        auto source = std::make_unique<FakeCameraSource>(
            FakeCameraConfig{{10, 20, 30}});
        FakeCameraSource* sourceView = source.get();
        AsyncCapture<int> capture(std::move(source));
        expect(capture.start(), "capture must start");
        expect(waitUntil([&] { return capture.state() == CaptureState::STOPPED; }),
               "finite source must finish");
        const auto metrics = capture.metrics();
        expect(metrics.capturedFrames == 3, "captured count must be exact");
        expect(metrics.overwrittenFrames == 2,
               "unconsumed finite frames must be overwritten");
        auto latest = capture.tryTakeLatest();
        expect(latest.has_value() && latest->value == 30,
               "consumer must receive newest finite frame");
        expect(latest->metadata.captureSequence == 3,
               "capture sequence must count every successful read");
        expect(!capture.tryTakeLatest().has_value(),
               "consumer must not process a frame twice");

        expect(capture.start(), "capture restart must be supported");
        expect(waitUntil([&] { return capture.state() == CaptureState::STOPPED; }),
               "restarted finite source must finish");
        expect(sourceView->openCount() == 2 && sourceView->closeCount() == 2,
               "restart must reopen and close on producer thread");
    }

    {
        FakeCameraConfig config;
        config.frames = {1};
        config.fatalAfterFrames = 1;
        auto source = std::make_unique<FakeCameraSource>(config);
        AsyncCapture<int> capture(std::move(source));
        capture.start();
        expect(waitUntil([&] { return capture.state() == CaptureState::FAILED; }),
               "fatal source failure must enter FAILED");
        expect(capture.metrics().captureFailures == 1,
               "fatal read must increment capture failure count");
        expect(!capture.lastError().empty(), "fatal read must retain error");
        capture.stop();
    }

    {
        FakeCameraConfig config;
        config.blockAfterFrames = true;
        auto source = std::make_unique<FakeCameraSource>(config);
        FakeCameraSource* sourceView = source.get();
        AsyncCapture<int> capture(std::move(source));
        capture.start();
        expect(waitUntil([&] { return capture.state() == CaptureState::RUNNING; }),
               "blocking fake must enter RUNNING");
        const auto stopStart = std::chrono::steady_clock::now();
        capture.stop();
        const auto stopDuration = std::chrono::steady_clock::now() - stopStart;
        expect(stopDuration < 500ms,
               "requestStop must cancel a synthetic blocking read");
        expect(capture.state() == CaptureState::STOPPED,
               "blocking stop must finish cleanly");
        expect(sourceView->closeCount() == 1,
               "blocking source must close exactly once");
    }

    {
        std::vector<int> frames(250);
        std::iota(frames.begin(), frames.end(), 1);
        auto source = std::make_unique<FakeCameraSource>(
            FakeCameraConfig{frames});
        AsyncCapture<int> capture(std::move(source));
        capture.start();

        std::vector<CapturedFrameMetadata> consumed;
        while (true) {
            auto latest = capture.tryTakeLatest();
            if (latest.has_value()) {
                consumed.push_back(latest->metadata);
                std::this_thread::sleep_for(2ms);
                continue;
            }
            if (capture.state() == CaptureState::STOPPED) break;
            std::this_thread::yield();
        }

        const auto metrics = capture.metrics();
        expect(metrics.capturedFrames == 250,
               "fast producer must count every captured frame");
        expect(metrics.overwrittenFrames > 0,
               "slow consumer must cause observable overwrites");
        expect(!consumed.empty() &&
                   consumed.back().captureSequence == metrics.capturedFrames,
               "slow consumer must converge on final producer sequence");
        expect(consumed.size() < metrics.capturedFrames,
               "consumer must skip frames instead of building a queue");
        for (std::size_t i = 1; i < consumed.size(); ++i) {
            expect(consumed[i].captureSequence >
                       consumed[i - 1].captureSequence,
                   "consumed sequences must increase");
            expect(consumed[i].captureTimestampUs >
                       consumed[i - 1].captureTimestampUs,
                   "capture timestamps must remain strictly monotonic");
        }
    }

    {
        FakeCameraConfig config;
        config.frames = {1, 2, 3, 4, 5, 6, 7, 8};
        config.frameDelay = 3ms;
        auto source = std::make_unique<FakeCameraSource>(config);
        AsyncCapture<int> capture(std::move(source));
        capture.start();
        std::vector<CapturedFrameMetadata> consumed;
        while (capture.state() != CaptureState::STOPPED ||
               capture.metrics().capturedFrames < config.frames.size()) {
            if (auto latest = capture.tryTakeLatest())
                consumed.push_back(latest->metadata);
            else
                std::this_thread::sleep_for(1ms);
        }
        if (auto latest = capture.tryTakeLatest())
            consumed.push_back(latest->metadata);
        expect(consumed.size() >= 2,
               "paced source must expose multiple consumed timestamps");
        for (std::size_t i = 1; i < consumed.size(); ++i) {
            expect(consumed[i].captureTimestampUs >
                       consumed[i - 1].captureTimestampUs,
                   "paced timestamps must be strictly increasing");
        }
    }

    return 0;
}
