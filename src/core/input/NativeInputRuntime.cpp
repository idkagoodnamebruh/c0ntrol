#include "NativeInputRuntime.h"

#include <algorithm>
#include <utility>

#include "src/core/actions/ActionDispatcher.h"

namespace {

bool isReleaseEvent(const GestureEvent& event) {
    return event.type == GestureEventType::PINCH_END ||
           event.type == GestureEventType::PINCH_CANCEL;
}

} // namespace

NativeInputRuntime::NativeInputRuntime(
    std::unique_ptr<ISystemInputBackend> backend,
    PointerMappingConfig pointerConfig,
    InputConfig inputConfig)
    : m_pointerConfig(sanitizePointerMappingConfig(pointerConfig)),
      m_inputConfig([&inputConfig] {
          InputConfig sanitized = sanitizeInputConfig(inputConfig);
          sanitized.enabled = false;
          return sanitized;
      }()),
      m_worker([this, ownedBackend = std::move(backend)](
                   std::stop_token stopToken) mutable {
          workerMain(stopToken, std::move(ownedBackend));
      }) {}

NativeInputRuntime::~NativeInputRuntime() {
    shutdown();
}

bool NativeInputRuntime::containsRelease(
    const GesturePipelineResult& result, Handedness hand) {
    for (std::size_t i = 0; i < result.events.count; ++i) {
        const GestureEvent& event = result.events.events[i];
        if (isReleaseEvent(event) &&
            (hand == Handedness::UNKNOWN || event.handedness == hand)) {
            return true;
        }
    }
    return false;
}

void NativeInputRuntime::clearPendingLocked() {
    m_latestFrame.reset();
    m_semanticFrames.clear();
}

void NativeInputRuntime::publishStateLocked(NativeInputState state,
                                            std::string error) {
    m_state = state;
    m_error = std::move(error);
    m_condition.notify_all();
}

void NativeInputRuntime::requestEnabledLocked(bool enabled) {
    m_inputConfig.enabled = enabled;
    if (enabled) {
        if (m_desiredEnabled &&
            (m_state == NativeInputState::ACTIVATING ||
             m_state == NativeInputState::READY)) {
            return;
        }
        ++m_generation;
        m_desiredEnabled = true;
        m_configurationDirty = true;
        clearPendingLocked();
        publishStateLocked(NativeInputState::ACTIVATING);
        return;
    }

    if (!m_desiredEnabled && m_state == NativeInputState::DISABLED) {
        clearPendingLocked();
        return;
    }
    ++m_generation;
    m_desiredEnabled = false;
    m_configurationDirty = false;
    clearPendingLocked();
    if (m_state == NativeInputState::FAILED ||
        m_state == NativeInputState::DISABLED) {
        publishStateLocked(NativeInputState::DISABLED);
    } else {
        publishStateLocked(NativeInputState::STOPPING);
    }
}

void NativeInputRuntime::requestConfiguration(
    PointerMappingConfig pointerConfig, InputConfig inputConfig) {
    std::lock_guard lock(m_mutex);
    if (m_shutdownRequested) return;

    m_pointerConfig = sanitizePointerMappingConfig(pointerConfig);
    inputConfig = sanitizeInputConfig(inputConfig);
    const bool requestedEnabled = inputConfig.enabled;
    m_inputConfig = inputConfig;

    if (requestedEnabled == m_desiredEnabled &&
        (m_state == NativeInputState::ACTIVATING ||
         m_state == NativeInputState::READY)) {
        m_configurationDirty = true;
        m_condition.notify_all();
        return;
    }
    requestEnabledLocked(requestedEnabled);
    m_condition.notify_all();
}

void NativeInputRuntime::requestEnabled(bool enabled) {
    std::lock_guard lock(m_mutex);
    if (m_shutdownRequested) return;
    requestEnabledLocked(enabled);
    m_condition.notify_all();
}

void NativeInputRuntime::enqueueSemanticLocked(
    const GesturePipelineResult& result) {
    for (Handedness hand : {Handedness::LEFT, Handedness::RIGHT}) {
        if (!containsRelease(result, hand)) continue;
        for (auto it = m_semanticFrames.begin();
             it != m_semanticFrames.end();) {
            if (containsRelease(*it, hand)) {
                it = m_semanticFrames.erase(it);
                ++m_droppedSemanticFrameCount;
            } else {
                ++it;
            }
        }
    }

    if (m_semanticFrames.size() >= kMaxPendingSemanticFrames) {
        const auto droppable = std::find_if(
            m_semanticFrames.begin(), m_semanticFrames.end(),
            [](const GesturePipelineResult& pending) {
                return !containsRelease(pending);
            });
        if (droppable != m_semanticFrames.end()) {
            m_semanticFrames.erase(droppable);
            ++m_droppedSemanticFrameCount;
        } else if (!containsRelease(result)) {
            ++m_droppedSemanticFrameCount;
            return;
        } else {
            // Duplicate releases were coalesced above. This fallback remains
            // bounded and prefers the newest safety release.
            m_semanticFrames.pop_front();
            ++m_droppedSemanticFrameCount;
        }
    }
    m_semanticFrames.push_back(result);
}

void NativeInputRuntime::submitLatest(
    const GesturePipelineResult& result) {
    std::lock_guard lock(m_mutex);
    if (m_shutdownRequested || !m_desiredEnabled ||
        m_state != NativeInputState::READY) {
        return;
    }

    if (result.events.count > 0) {
        enqueueSemanticLocked(result);
    } else {
        if (m_latestFrame.has_value()) ++m_droppedFrameCount;
        m_latestFrame = result;
    }
    m_condition.notify_all();
}

NativeInputStatus NativeInputRuntime::status() const {
    std::lock_guard lock(m_mutex);
    return {m_state,
            m_desiredEnabled,
            m_generation,
            m_error,
            m_latestFrame.has_value() ? 1U : 0U,
            m_semanticFrames.size(),
            m_droppedFrameCount,
            m_droppedSemanticFrameCount};
}

bool NativeInputRuntime::waitForState(
    NativeInputState expected, std::chrono::milliseconds timeout) const {
    std::unique_lock lock(m_mutex);
    return m_condition.wait_for(lock, timeout, [this, expected] {
        return m_state == expected;
    });
}

void NativeInputRuntime::workerMain(
    std::stop_token stopToken,
    std::unique_ptr<ISystemInputBackend> backend) {
    if (!backend) {
        std::lock_guard lock(m_mutex);
        m_desiredEnabled = false;
        publishStateLocked(NativeInputState::FAILED,
                           "native input backend is unavailable");
        return;
    }

    PointerMappingConfig initialPointerConfig;
    InputConfig disabledConfig;
    {
        std::lock_guard lock(m_mutex);
        initialPointerConfig = m_pointerConfig;
        disabledConfig = m_inputConfig;
        disabledConfig.enabled = false;
    }
    ActionDispatcher dispatcher(*backend, initialPointerConfig,
                                disabledConfig);
    (void)dispatcher.initialize();
    bool operational = false;

    auto resetDispatcher = [&] {
        PointerMappingConfig latestPointer;
        InputConfig latestDisabled;
        {
            std::lock_guard lock(m_mutex);
            latestPointer = m_pointerConfig;
            latestDisabled = m_inputConfig;
            latestDisabled.enabled = false;
        }
        dispatcher.shutdown();
        (void)dispatcher.applyConfiguration(latestPointer, latestDisabled);
        (void)dispatcher.initialize();
        operational = false;
    };

    while (true) {
        enum class Work { NONE, ACTIVATE, DISABLE, CONFIGURE, DISPATCH, STOP };
        Work work = Work::NONE;
        std::uint64_t generation = 0;
        PointerMappingConfig pointerConfig;
        InputConfig inputConfig;
        GesturePipelineResult pipelineResult;

        {
            std::unique_lock lock(m_mutex);
            m_condition.wait(lock, [this, stopToken, operational] {
                return m_shutdownRequested || stopToken.stop_requested() ||
                    (!m_desiredEnabled &&
                     (operational ||
                      m_state == NativeInputState::STOPPING)) ||
                    (m_desiredEnabled && !operational &&
                     m_state == NativeInputState::ACTIVATING) ||
                    (m_desiredEnabled && operational &&
                     m_configurationDirty) ||
                    (m_desiredEnabled && operational &&
                     (!m_semanticFrames.empty() ||
                      m_latestFrame.has_value()));
            });

            if (m_shutdownRequested || stopToken.stop_requested()) {
                work = Work::STOP;
            } else if (!m_desiredEnabled &&
                       (operational ||
                        m_state == NativeInputState::STOPPING)) {
                work = Work::DISABLE;
            } else if (m_desiredEnabled && !operational &&
                       m_state == NativeInputState::ACTIVATING) {
                work = Work::ACTIVATE;
                generation = m_generation;
                pointerConfig = m_pointerConfig;
                inputConfig = m_inputConfig;
                m_configurationDirty = false;
            } else if (m_desiredEnabled && operational &&
                       m_configurationDirty) {
                work = Work::CONFIGURE;
                generation = m_generation;
                pointerConfig = m_pointerConfig;
                inputConfig = m_inputConfig;
                m_configurationDirty = false;
                clearPendingLocked();
            } else if (m_desiredEnabled && operational) {
                work = Work::DISPATCH;
                generation = m_generation;
                if (!m_semanticFrames.empty()) {
                    pipelineResult = m_semanticFrames.front();
                    m_semanticFrames.pop_front();
                } else if (m_latestFrame.has_value()) {
                    pipelineResult = *m_latestFrame;
                    m_latestFrame.reset();
                } else {
                    work = Work::NONE;
                }
            }
        }

        if (work == Work::STOP) {
            dispatcher.shutdown();
            std::lock_guard lock(m_mutex);
            operational = false;
            m_desiredEnabled = false;
            clearPendingLocked();
            publishStateLocked(NativeInputState::DISABLED);
            return;
        }

        if (work == Work::DISABLE) {
            resetDispatcher();
            std::lock_guard lock(m_mutex);
            if (!m_desiredEnabled)
                publishStateLocked(NativeInputState::DISABLED);
            else
                publishStateLocked(NativeInputState::ACTIVATING);
            continue;
        }

        if (work == Work::ACTIVATE) {
            InputConfig activationConfig = inputConfig;
            activationConfig.enabled = false;
            bool success = dispatcher.applyConfiguration(pointerConfig,
                                                         activationConfig);
            if (success) success = dispatcher.setInputEnabled(true);
            std::string error = dispatcher.lastError();
            bool stale = false;
            bool adopted = false;

            // Initialization can be interactive. Once it returns, repeatedly
            // apply the newest requested configuration until it is stable.
            // READY is adopted while holding the same mutex used by callers,
            // closing the late-disable/config race at the publication edge.
            while (success && !stale && !adopted) {
                PointerMappingConfig latestPointer;
                InputConfig latestInput;
                {
                    std::lock_guard lock(m_mutex);
                    stale = m_shutdownRequested || !m_desiredEnabled ||
                            generation != m_generation;
                    if (!stale) {
                        latestPointer = m_pointerConfig;
                        latestInput = m_inputConfig;
                        m_configurationDirty = false;
                    }
                }
                if (stale) break;

                latestInput.enabled = true;
                success = dispatcher.applyConfiguration(latestPointer,
                                                        latestInput) &&
                          dispatcher.resetRuntimeState();
                error = dispatcher.lastError();
                if (!success) break;

                std::lock_guard lock(m_mutex);
                stale = m_shutdownRequested || !m_desiredEnabled ||
                        generation != m_generation;
                if (!stale && !m_configurationDirty) {
                    operational = true;
                    clearPendingLocked();
                    publishStateLocked(NativeInputState::READY);
                    adopted = true;
                }
            }

            if (adopted) continue;

            if (stale) {
                resetDispatcher();
                std::lock_guard lock(m_mutex);
                clearPendingLocked();
                if (m_shutdownRequested || !m_desiredEnabled)
                    publishStateLocked(NativeInputState::DISABLED);
                else
                    publishStateLocked(NativeInputState::ACTIVATING);
                continue;
            }

            if (!success) {
                resetDispatcher();
                std::lock_guard lock(m_mutex);
                if (generation == m_generation && m_desiredEnabled) {
                    m_desiredEnabled = false;
                    m_inputConfig.enabled = false;
                    clearPendingLocked();
                    publishStateLocked(NativeInputState::FAILED,
                        error.empty() ? "native input activation failed"
                                      : error);
                }
                continue;
            }
        }

        if (work == Work::CONFIGURE) {
            inputConfig.enabled = true;
            const bool success =
                dispatcher.applyConfiguration(pointerConfig, inputConfig) &&
                dispatcher.resetRuntimeState();
            if (!success) {
                const std::string error = dispatcher.lastError();
                resetDispatcher();
                std::lock_guard lock(m_mutex);
                if (generation == m_generation) {
                    m_desiredEnabled = false;
                    m_inputConfig.enabled = false;
                    publishStateLocked(NativeInputState::FAILED, error);
                }
            }
            continue;
        }

        if (work == Work::DISPATCH) {
            const ActionDispatchResult result = dispatcher.process(pipelineResult);
            if (!result.success) {
                const std::string error = result.error;
                resetDispatcher();
                std::lock_guard lock(m_mutex);
                if (generation == m_generation) {
                    m_desiredEnabled = false;
                    m_inputConfig.enabled = false;
                    clearPendingLocked();
                    publishStateLocked(NativeInputState::FAILED, error);
                }
            }
        }
    }
}

void NativeInputRuntime::shutdown() {
    {
        std::lock_guard lock(m_mutex);
        if (!m_worker.joinable()) return;
        m_shutdownRequested = true;
        m_desiredEnabled = false;
        ++m_generation;
        clearPendingLocked();
        publishStateLocked(NativeInputState::STOPPING);
    }
    m_worker.request_stop();
    m_condition.notify_all();
    m_worker.join();
}
