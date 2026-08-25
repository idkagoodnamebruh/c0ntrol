#ifndef RECORDINGSYSTEMINPUTBACKEND_H
#define RECORDINGSYSTEMINPUTBACKEND_H

#include <string>
#include <vector>

#include "src/core/actions/ISystemInputBackend.h"

enum class RecordedInputType { MOVE, BUTTON_DOWN, BUTTON_UP, SCROLL };

struct RecordedInput {
    RecordedInputType type{RecordedInputType::MOVE};
    DesktopPoint point{};
    int scrollNotches{0};
};

class RecordingSystemInputBackend final : public ISystemInputBackend {
public:
    explicit RecordingSystemInputBackend(
        DesktopGeometry geometry = {0, 0, 1920, 1080})
        : m_geometry(geometry) {}

    bool initialize() override {
        ++initializeCount;
        m_initialized = m_geometry.isValid() && !failInitialize;
        m_lastError = m_initialized ? "" : "recording backend initialize failure";
        return m_initialized;
    }
    DesktopGeometry desktopGeometry() const override { return m_geometry; }
    bool movePointer(const DesktopPoint& point) override {
        if (!m_initialized || failNextMove) {
            failNextMove = false;
            m_lastError = "recording backend move failure";
            return false;
        }
        records.push_back({RecordedInputType::MOVE, point, 0});
        return true;
    }
    bool primaryButtonDown() override {
        if (!m_initialized || failNextDown) {
            failNextDown = false;
            m_lastError = "recording backend button-down failure";
            return false;
        }
        records.push_back({RecordedInputType::BUTTON_DOWN, {}, 0});
        ++buttonDownCount;
        return true;
    }
    bool primaryButtonUp() override {
        if (!m_initialized || failNextUp) {
            failNextUp = false;
            m_lastError = "recording backend button-up failure";
            return false;
        }
        records.push_back({RecordedInputType::BUTTON_UP, {}, 0});
        ++buttonUpCount;
        return true;
    }
    bool scrollVertical(int notches) override {
        if (!m_initialized || failNextScroll) {
            failNextScroll = false;
            m_lastError = "recording backend scroll failure";
            return false;
        }
        records.push_back({RecordedInputType::SCROLL, {}, notches});
        ++scrollCount;
        return true;
    }
    void shutdown() override {
        shutdownCalled = true;
        m_initialized = false;
    }
    std::string lastError() const override { return m_lastError; }

    std::vector<RecordedInput> records;
    int buttonDownCount{0};
    int buttonUpCount{0};
    int scrollCount{0};
    int initializeCount{0};
    bool failInitialize{false};
    bool failNextMove{false};
    bool failNextDown{false};
    bool failNextUp{false};
    bool failNextScroll{false};
    bool shutdownCalled{false};

private:
    DesktopGeometry m_geometry;
    bool m_initialized{false};
    std::string m_lastError;
};

#endif // RECORDINGSYSTEMINPUTBACKEND_H
