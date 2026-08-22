#ifndef PIPELINEMETRICS_H
#define PIPELINEMETRICS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>

#include "src/core/capture/CaptureTypes.h"

class SlidingWindowRate {
public:
    explicit SlidingWindowRate(std::int64_t windowUs = 1'000'000,
                               std::size_t maxSamples = 240)
        : m_windowUs(std::max<std::int64_t>(1, windowUs)),
          m_maxSamples(std::max<std::size_t>(2, maxSamples)) {}

    void record(std::int64_t timestampUs) {
        if (!m_samples.empty() && timestampUs <= m_samples.back())
            timestampUs = m_samples.back() + 1;
        m_samples.push_back(timestampUs);
        while (m_samples.size() > m_maxSamples ||
               (m_samples.size() > 2 &&
                m_samples.back() - m_samples.front() > m_windowUs)) {
            m_samples.pop_front();
        }
    }

    double rate() const {
        if (m_samples.size() < 2) return 0.0;
        const auto elapsedUs = m_samples.back() - m_samples.front();
        if (elapsedUs <= 0) return 0.0;
        return static_cast<double>(m_samples.size() - 1) * 1'000'000.0 /
               static_cast<double>(elapsedUs);
    }

    std::size_t sampleCount() const { return m_samples.size(); }
    void reset() { m_samples.clear(); }

private:
    std::int64_t m_windowUs;
    std::size_t m_maxSamples;
    std::deque<std::int64_t> m_samples;
};

struct PipelineMetrics {
    double captureFps{0.0};
    double processingFps{0.0};
    std::uint64_t capturedFrames{0};
    std::uint64_t processedFrames{0};
    std::uint64_t overwrittenFrames{0};
    std::uint64_t captureFailures{0};
    std::int64_t frameAgeAtProcessingUs{0};
    std::int64_t inferenceDurationUs{0};
    std::int64_t processingDurationUs{0};
    CaptureState captureState{CaptureState::STOPPED};
};

class PipelineMetricsTracker {
public:
    void reset() {
        m_processingRate.reset();
        m_processedFrames = 0;
        m_frameAgeUs = 0;
        m_inferenceDurationUs = 0;
        m_processingDurationUs = 0;
    }

    void recordProcessed(std::int64_t captureTimestampUs,
                         std::int64_t processingStartUs,
                         std::int64_t inferenceStartUs,
                         std::int64_t inferenceEndUs,
                         std::int64_t processingEndUs) {
        ++m_processedFrames;
        m_frameAgeUs = std::max<std::int64_t>(
            0, processingStartUs - captureTimestampUs);
        m_inferenceDurationUs = std::max<std::int64_t>(
            0, inferenceEndUs - inferenceStartUs);
        m_processingDurationUs = std::max<std::int64_t>(
            0, processingEndUs - processingStartUs);
        m_processingRate.record(processingEndUs);
    }

    PipelineMetrics snapshot(const CaptureMetrics& capture) const {
        PipelineMetrics result;
        result.captureFps = capture.captureFps;
        result.processingFps = m_processingRate.rate();
        result.capturedFrames = capture.capturedFrames;
        result.processedFrames = m_processedFrames;
        result.overwrittenFrames = capture.overwrittenFrames;
        result.captureFailures = capture.captureFailures;
        result.frameAgeAtProcessingUs = m_frameAgeUs;
        result.inferenceDurationUs = m_inferenceDurationUs;
        result.processingDurationUs = m_processingDurationUs;
        result.captureState = capture.state;
        return result;
    }

private:
    SlidingWindowRate m_processingRate;
    std::uint64_t m_processedFrames{0};
    std::int64_t m_frameAgeUs{0};
    std::int64_t m_inferenceDurationUs{0};
    std::int64_t m_processingDurationUs{0};
};

#endif // PIPELINEMETRICS_H
