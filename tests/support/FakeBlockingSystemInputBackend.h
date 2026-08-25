#ifndef FAKEBLOCKINGSYSTEMINPUTBACKEND_H
#define FAKEBLOCKINGSYSTEMINPUTBACKEND_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "src/core/actions/ISystemInputBackend.h"
#include "src/core/actions/RecordingSystemInputBackend.h"

struct FakeBlockingInputSnapshot {
    int initializeCount{0};
    int shutdownCount{0};
    int moveCount{0};
    int buttonDownCount{0};
    int buttonUpCount{0};
    int scrollCount{0};
    int moveEnteredCount{0};
    int ownerThreadViolations{0};
    bool initialized{false};
    std::vector<RecordedInput> records;
};

class FakeBlockingInputState final {
public:
    void completeInitialize(int attempt, bool success) {
        std::lock_guard lock(m_mutex);
        if (attempt <= 0) return;
        const std::size_t index = static_cast<std::size_t>(attempt - 1);
        if (m_initializeOutcomes.size() <= index)
            m_initializeOutcomes.resize(index + 1);
        m_initializeOutcomes[index] = success;
        m_condition.notify_all();
    }

    bool waitForInitializeCount(
        int count, std::chrono::milliseconds timeout) const {
        std::unique_lock lock(m_mutex);
        return m_condition.wait_for(lock, timeout, [this, count] {
            return m_snapshot.initializeCount >= count;
        });
    }

    bool waitForMoveEnteredCount(
        int count, std::chrono::milliseconds timeout) const {
        std::unique_lock lock(m_mutex);
        return m_condition.wait_for(lock, timeout, [this, count] {
            return m_snapshot.moveEnteredCount >= count;
        });
    }

    bool waitForMoveCount(
        int count, std::chrono::milliseconds timeout) const {
        std::unique_lock lock(m_mutex);
        return m_condition.wait_for(lock, timeout, [this, count] {
            return m_snapshot.moveCount >= count;
        });
    }

    bool waitForButtonDownCount(
        int count, std::chrono::milliseconds timeout) const {
        std::unique_lock lock(m_mutex);
        return m_condition.wait_for(lock, timeout, [this, count] {
            return m_snapshot.buttonDownCount >= count;
        });
    }

    bool waitForButtonUpCount(
        int count, std::chrono::milliseconds timeout) const {
        std::unique_lock lock(m_mutex);
        return m_condition.wait_for(lock, timeout, [this, count] {
            return m_snapshot.buttonUpCount >= count;
        });
    }

    void setMoveBlocked(bool blocked) {
        std::lock_guard lock(m_mutex);
        m_moveBlocked = blocked;
        m_condition.notify_all();
    }

    FakeBlockingInputSnapshot snapshot() const {
        std::lock_guard lock(m_mutex);
        return m_snapshot;
    }

private:
    friend class FakeBlockingSystemInputBackend;

    void noteOwnerThreadLocked() {
        const std::thread::id caller = std::this_thread::get_id();
        if (!m_ownerThread.has_value()) {
            m_ownerThread = caller;
        } else if (*m_ownerThread != caller) {
            ++m_snapshot.ownerThreadViolations;
        }
    }

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    FakeBlockingInputSnapshot m_snapshot;
    std::vector<std::optional<bool>> m_initializeOutcomes;
    std::optional<std::thread::id> m_ownerThread;
    bool m_moveBlocked{false};
    std::string m_lastError;
};

class FakeBlockingSystemInputBackend final : public ISystemInputBackend {
public:
    explicit FakeBlockingSystemInputBackend(
        std::shared_ptr<FakeBlockingInputState> state,
        DesktopGeometry geometry = {0, 0, 1000, 1000})
        : m_state(std::move(state)), m_geometry(geometry) {}

    bool initialize() override {
        std::unique_lock lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        const int attempt = ++m_state->m_snapshot.initializeCount;
        m_state->m_condition.notify_all();
        m_state->m_condition.wait(lock, [this, attempt] {
            const std::size_t index = static_cast<std::size_t>(attempt - 1);
            return m_state->m_initializeOutcomes.size() > index &&
                   m_state->m_initializeOutcomes[index].has_value();
        });
        const bool success =
            *m_state->m_initializeOutcomes[static_cast<std::size_t>(attempt - 1)];
        m_state->m_snapshot.initialized = success && m_geometry.isValid();
        m_state->m_lastError = m_state->m_snapshot.initialized
            ? std::string{}
            : "fake blocking initialization failed";
        m_state->m_condition.notify_all();
        return m_state->m_snapshot.initialized;
    }

    DesktopGeometry desktopGeometry() const override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        return m_geometry;
    }

    bool movePointer(const DesktopPoint& point) override {
        std::unique_lock lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        if (!m_state->m_snapshot.initialized) return failLocked("move");
        ++m_state->m_snapshot.moveEnteredCount;
        m_state->m_condition.notify_all();
        m_state->m_condition.wait(lock, [this] {
            return !m_state->m_moveBlocked;
        });
        if (!m_state->m_snapshot.initialized) return failLocked("move");
        ++m_state->m_snapshot.moveCount;
        m_state->m_snapshot.records.push_back(
            {RecordedInputType::MOVE, point, 0});
        m_state->m_condition.notify_all();
        return true;
    }

    bool primaryButtonDown() override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        if (!m_state->m_snapshot.initialized) return failLocked("button down");
        ++m_state->m_snapshot.buttonDownCount;
        m_state->m_snapshot.records.push_back(
            {RecordedInputType::BUTTON_DOWN, {}, 0});
        m_state->m_condition.notify_all();
        return true;
    }

    bool primaryButtonUp() override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        if (!m_state->m_snapshot.initialized) return failLocked("button up");
        ++m_state->m_snapshot.buttonUpCount;
        m_state->m_snapshot.records.push_back(
            {RecordedInputType::BUTTON_UP, {}, 0});
        m_state->m_condition.notify_all();
        return true;
    }

    bool scrollVertical(int notches) override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        if (!m_state->m_snapshot.initialized) return failLocked("scroll");
        ++m_state->m_snapshot.scrollCount;
        m_state->m_snapshot.records.push_back(
            {RecordedInputType::SCROLL, {}, notches});
        m_state->m_condition.notify_all();
        return true;
    }

    void shutdown() override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        ++m_state->m_snapshot.shutdownCount;
        m_state->m_snapshot.initialized = false;
        m_state->m_condition.notify_all();
    }

    std::string lastError() const override {
        std::lock_guard lock(m_state->m_mutex);
        m_state->noteOwnerThreadLocked();
        return m_state->m_lastError;
    }

private:
    bool failLocked(const char* operation) {
        m_state->m_lastError =
            std::string("fake backend is not initialized for ") + operation;
        return false;
    }

    std::shared_ptr<FakeBlockingInputState> m_state;
    DesktopGeometry m_geometry;
};

#endif // FAKEBLOCKINGSYSTEMINPUTBACKEND_H
