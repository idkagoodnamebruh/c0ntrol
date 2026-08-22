#include <cmath>
#include <cstdlib>
#include <iostream>

#include "src/core/metrics/PipelineMetrics.h"

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    SlidingWindowRate rate(1'000'000, 4);
    expect(rate.rate() == 0.0, "empty rate must be zero");
    rate.record(0);
    expect(rate.rate() == 0.0, "first sample must be stable");
    rate.record(500'000);
    rate.record(1'000'000);
    expect(std::abs(rate.rate() - 2.0) < 0.0001,
           "three samples across one second must report 2 FPS");
    rate.record(1'500'000);
    rate.record(2'000'000);
    expect(rate.sampleCount() <= 4, "FPS window must be memory bounded");

    PipelineMetricsTracker tracker;
    CaptureMetrics capture;
    capture.captureFps = 30.0;
    capture.capturedFrames = 10;
    capture.overwrittenFrames = 3;
    capture.captureFailures = 1;
    capture.state = CaptureState::RUNNING;

    tracker.recordProcessed(1'000, 1'500, 1'600, 2'100, 2'500);
    PipelineMetrics metrics = tracker.snapshot(capture);
    expect(metrics.capturedFrames == 10, "captured count must propagate");
    expect(metrics.processedFrames == 1, "processed count must increment");
    expect(metrics.overwrittenFrames == 3, "overwrite count must propagate");
    expect(metrics.captureFailures == 1, "failure count must propagate");
    expect(metrics.frameAgeAtProcessingUs == 500,
           "frame age must use processing start minus capture completion");
    expect(metrics.inferenceDurationUs == 500,
           "inference duration must be measured independently");
    expect(metrics.processingDurationUs == 1'000,
           "processing duration must cover the full processing interval");

    tracker.recordProcessed(5'000, 4'000, 4'500, 4'000, 3'000);
    metrics = tracker.snapshot(capture);
    expect(metrics.frameAgeAtProcessingUs >= 0,
           "frame age must never be negative");
    expect(metrics.inferenceDurationUs >= 0,
           "inference duration must never be negative");
    expect(metrics.processingDurationUs >= 0,
           "processing duration must never be negative");
    return 0;
}
