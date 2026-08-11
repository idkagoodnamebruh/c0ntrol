#ifndef FRAMESYNCHRONIZER_H
#define FRAMESYNCHRONIZER_H

#include <chrono>

class FrameSynchronizer {
public:
    FrameSynchronizer(int targetFps = 30)
        : m_frameDurationMs(1000 / targetFps), m_lastFrameTime(std::chrono::steady_clock::now()) {}

    void sync() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFrameTime).count();
        if (elapsed < m_frameDurationMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_frameDurationMs - elapsed));
        }
        m_lastFrameTime = std::chrono::steady_clock::now();
    }

private:
    int m_frameDurationMs;
    std::chrono::steady_clock::time_point m_lastFrameTime;
};

#endif // FRAMESYNCHRONIZER_H
