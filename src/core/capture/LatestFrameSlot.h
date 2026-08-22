#ifndef LATESTFRAMESLOT_H
#define LATESTFRAMESLOT_H

#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

#include "src/core/capture/CaptureTypes.h"

struct LatestFrameSlotStats {
    std::uint64_t publishedFrames{0};
    std::uint64_t consumedFrames{0};
    std::uint64_t overwrittenFrames{0};
    std::uint64_t lastConsumedSequence{0};
    bool hasPendingFrame{false};
};

template <typename T>
class LatestFrameSlot {
public:
    void publish(CapturedFrame<T> frame) {
        std::lock_guard lock(m_mutex);
        if (m_latest.has_value()) ++m_stats.overwrittenFrames;
        m_latest.reset();
        m_latest.emplace(std::move(frame));
        ++m_stats.publishedFrames;
        m_stats.hasPendingFrame = true;
    }

    std::optional<CapturedFrame<T>> consumeLatest() {
        std::lock_guard lock(m_mutex);
        if (!m_latest.has_value()) return std::nullopt;

        std::optional<CapturedFrame<T>> result;
        result.emplace(std::move(*m_latest));
        m_latest.reset();
        ++m_stats.consumedFrames;
        m_stats.lastConsumedSequence = result->metadata.captureSequence;
        m_stats.hasPendingFrame = false;
        return result;
    }

    LatestFrameSlotStats stats() const {
        std::lock_guard lock(m_mutex);
        return m_stats;
    }

    void reset() {
        std::lock_guard lock(m_mutex);
        m_latest.reset();
        m_stats = {};
    }

private:
    mutable std::mutex m_mutex;
    std::optional<CapturedFrame<T>> m_latest;
    LatestFrameSlotStats m_stats;
};

#endif // LATESTFRAMESLOT_H
