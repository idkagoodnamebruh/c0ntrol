#ifndef FAKECAMERASOURCE_H
#define FAKECAMERASOURCE_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "src/core/capture/ICameraSource.h"

struct FakeCameraConfig {
    std::vector<int> frames;
    std::chrono::milliseconds frameDelay{0};
    bool blockAfterFrames{false};
    bool failOpen{false};
    std::optional<std::size_t> fatalAfterFrames;
};

class FakeCameraSource final : public ICameraSource<int> {
public:
    explicit FakeCameraSource(FakeCameraConfig config)
        : m_config(std::move(config)) {}

    bool open(std::string& error) override {
        std::lock_guard lock(m_mutex);
        ++m_openCount;
        m_index = 0;
        m_stopRequested = false;
        if (m_config.failOpen) {
            error = "synthetic open failure";
            return false;
        }
        m_opened = true;
        return true;
    }

    CameraReadStatus read(int& frame, std::string& error) override {
        std::unique_lock lock(m_mutex);
        if (!m_opened) {
            error = "synthetic source is not open";
            return CameraReadStatus::FATAL_ERROR;
        }
        if (m_stopRequested) return CameraReadStatus::STOPPED;

        if (m_config.frameDelay.count() > 0) {
            m_cv.wait_for(lock, m_config.frameDelay,
                          [this] { return m_stopRequested; });
            if (m_stopRequested) return CameraReadStatus::STOPPED;
        }

        if (m_config.fatalAfterFrames.has_value() &&
            m_index >= *m_config.fatalAfterFrames) {
            error = "synthetic read failure";
            return CameraReadStatus::FATAL_ERROR;
        }

        if (m_index < m_config.frames.size()) {
            frame = m_config.frames[m_index++];
            return CameraReadStatus::FRAME;
        }

        if (!m_config.blockAfterFrames)
            return CameraReadStatus::END_OF_STREAM;

        m_cv.wait(lock, [this] { return m_stopRequested; });
        return CameraReadStatus::STOPPED;
    }

    void requestStop() override {
        {
            std::lock_guard lock(m_mutex);
            m_stopRequested = true;
        }
        m_cv.notify_all();
    }

    void close() override {
        std::lock_guard lock(m_mutex);
        if (m_opened) ++m_closeCount;
        m_opened = false;
    }

    std::size_t openCount() const {
        std::lock_guard lock(m_mutex);
        return m_openCount;
    }

    std::size_t closeCount() const {
        std::lock_guard lock(m_mutex);
        return m_closeCount;
    }

private:
    FakeCameraConfig m_config;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::size_t m_index{0};
    std::size_t m_openCount{0};
    std::size_t m_closeCount{0};
    bool m_opened{false};
    bool m_stopRequested{false};
};

#endif // FAKECAMERASOURCE_H
