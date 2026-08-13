#ifndef TRACKINGCLOCK_H
#define TRACKINGCLOCK_H

#include <chrono>
#include <cstdint>

class TrackingClock {
public:
    std::int64_t nextTimestampUs() {
        auto value = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        if (value <= m_lastTimestampUs) value = m_lastTimestampUs + 1;
        return m_lastTimestampUs = value;
    }

    std::uint64_t nextFrameId() { return m_nextFrameId++; }

private:
    std::int64_t m_lastTimestampUs{-1};
    std::uint64_t m_nextFrameId{0};
};

#endif // TRACKINGCLOCK_H
