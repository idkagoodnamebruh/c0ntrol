#ifndef ASYNCCAPTURE_H
#define ASYNCCAPTURE_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <utility>

#include "src/core/capture/ICameraSource.h"
#include "src/core/capture/LatestFrameSlot.h"
#include "src/core/metrics/PipelineMetrics.h"

struct AsyncCaptureConfig {
    unsigned int maxConsecutiveFailures{5};
    std::chrono::milliseconds failureBackoff{10};
};

template <typename T>
class AsyncCapture {
public:
    explicit AsyncCapture(std::unique_ptr<ICameraSource<T>> source,
                          AsyncCaptureConfig config = {})
        : m_source(std::move(source)), m_config(config) {
        m_config.maxConsecutiveFailures =
            std::max(1U, m_config.maxConsecutiveFailures);
    }

    ~AsyncCapture() { stop(); }

    AsyncCapture(const AsyncCapture&) = delete;
    AsyncCapture& operator=(const AsyncCapture&) = delete;

    bool start() {
        std::lock_guard lock(m_lifecycleMutex);
        const CaptureState current = m_state.load();
        if (current == CaptureState::STARTING ||
            current == CaptureState::RUNNING) {
            return true;
        }
        if (m_thread.joinable()) m_thread.join();

        m_slot.reset();
        {
            std::lock_guard metricsLock(m_metricsMutex);
            m_captureRate.reset();
            m_capturedFrames = 0;
            m_captureFailures = 0;
            m_lastCaptureTimestampUs = -1;
            m_nextSequence = 1;
            m_lastError.clear();
        }
        m_state.store(CaptureState::STARTING);
        m_thread = std::jthread(
            [this](std::stop_token token) { captureLoop(token); });
        return true;
    }

    void stop() {
        std::jthread thread;
        {
            std::lock_guard lock(m_lifecycleMutex);
            if (!m_thread.joinable()) {
                m_state.store(CaptureState::STOPPED);
                return;
            }
            m_state.store(CaptureState::STOPPING);
            m_source->requestStop();
            m_thread.request_stop();
            m_backoffCv.notify_all();
            thread = std::move(m_thread);
        }
        if (thread.joinable()) thread.join();
        m_state.store(CaptureState::STOPPED);
    }

    std::optional<CapturedFrame<T>> tryTakeLatest() {
        return m_slot.consumeLatest();
    }

    CaptureState state() const { return m_state.load(); }

    CaptureMetrics metrics() const {
        std::lock_guard lock(m_metricsMutex);
        CaptureMetrics result;
        result.captureFps = m_captureRate.rate();
        result.capturedFrames = m_capturedFrames;
        result.overwrittenFrames = m_slot.stats().overwrittenFrames;
        result.captureFailures = m_captureFailures;
        result.state = m_state.load();
        return result;
    }

    std::string lastError() const {
        std::lock_guard lock(m_metricsMutex);
        return m_lastError;
    }

private:
    static std::int64_t steadyNowUs() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::steady_clock::now().time_since_epoch())
            .count();
    }

    void captureLoop(std::stop_token token) {
        std::string error;
        if (!m_source->open(error)) {
            setFailure(error.empty() ? "camera source open failed" : error);
            return;
        }

        if (token.stop_requested()) {
            m_source->close();
            m_state.store(CaptureState::STOPPED);
            return;
        }

        m_state.store(CaptureState::RUNNING);
        unsigned int consecutiveFailures = 0;

        while (!token.stop_requested()) {
            T value{};
            error.clear();
            const CameraReadStatus status = m_source->read(value, error);
            if (status == CameraReadStatus::FRAME) {
                if (token.stop_requested()) break;
                consecutiveFailures = 0;
                CapturedFrameMetadata metadata;
                {
                    std::lock_guard lock(m_metricsMutex);
                    auto timestampUs = steadyNowUs();
                    if (timestampUs <= m_lastCaptureTimestampUs)
                        timestampUs = m_lastCaptureTimestampUs + 1;
                    m_lastCaptureTimestampUs = timestampUs;
                    metadata.captureTimestampUs = timestampUs;
                    metadata.captureSequence = m_nextSequence++;
                    ++m_capturedFrames;
                    m_captureRate.record(timestampUs);
                }
                m_slot.publish(
                    CapturedFrame<T>{std::move(value), metadata});
                continue;
            }

            if (status == CameraReadStatus::STOPPED ||
                status == CameraReadStatus::END_OF_STREAM) {
                break;
            }

            {
                std::lock_guard lock(m_metricsMutex);
                ++m_captureFailures;
                if (!error.empty()) m_lastError = error;
            }
            ++consecutiveFailures;
            if (status == CameraReadStatus::FATAL_ERROR ||
                consecutiveFailures >= m_config.maxConsecutiveFailures) {
                setFailure(error.empty() ? "camera source read failed" : error);
                break;
            }

            std::unique_lock waitLock(m_backoffMutex);
            m_backoffCv.wait_for(waitLock, m_config.failureBackoff, [this, &token] {
                return token.stop_requested() ||
                       m_state.load() == CaptureState::STOPPING;
            });
        }

        m_source->close();
        if (m_state.load() != CaptureState::FAILED)
            m_state.store(CaptureState::STOPPED);
    }

    void setFailure(const std::string& error) {
        {
            std::lock_guard lock(m_metricsMutex);
            m_lastError = error;
        }
        m_state.store(CaptureState::FAILED);
    }

    std::unique_ptr<ICameraSource<T>> m_source;
    AsyncCaptureConfig m_config;
    LatestFrameSlot<T> m_slot;
    std::atomic<CaptureState> m_state{CaptureState::STOPPED};
    std::jthread m_thread;
    mutable std::mutex m_lifecycleMutex;
    mutable std::mutex m_metricsMutex;
    std::mutex m_backoffMutex;
    std::condition_variable m_backoffCv;
    SlidingWindowRate m_captureRate;
    std::uint64_t m_capturedFrames{0};
    std::uint64_t m_captureFailures{0};
    std::uint64_t m_nextSequence{1};
    std::int64_t m_lastCaptureTimestampUs{-1};
    std::string m_lastError;
};

#endif // ASYNCCAPTURE_H
