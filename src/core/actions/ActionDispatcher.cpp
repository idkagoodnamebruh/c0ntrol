#include "ActionDispatcher.h"

ActionDispatcher::ActionDispatcher(ISystemInputBackend& backend,
                                   PointerMappingConfig mappingConfig,
                                   InputConfig inputConfig)
    : m_backend(backend),
      m_pointerMapper(mappingConfig),
      m_inputConfig(sanitizeInputConfig(inputConfig)) {}

ActionDispatcher::~ActionDispatcher() {
    shutdown();
}

bool ActionDispatcher::initialize() {
    if (m_initialized) return true;
    m_initialized = true;
    m_lastError.clear();
    if (!m_inputConfig.enabled) {
        m_inputEnabled = false;
        return true;
    }
    if (!ensureBackendInitialized()) {
        m_inputConfig.enabled = false;
        return false;
    }
    m_inputEnabled = true;
    return true;
}

bool ActionDispatcher::ensureBackendInitialized() {
    if (m_backendInitialized) return true;
    if (!m_backend.initialize()) {
        m_lastError = m_backend.lastError();
        m_inputEnabled = false;
        return false;
    }
    m_desktop = m_backend.desktopGeometry();
    if (!m_desktop.isValid()) {
        m_lastError = "system input backend returned invalid desktop geometry";
        m_backend.shutdown();
        m_inputEnabled = false;
        return false;
    }
    m_backendInitialized = true;
    m_lastError.clear();
    return true;
}

const GestureObservation* ActionDispatcher::selectActiveObservation(
    const GesturePipelineResult& pipelineResult) const {
    const GestureObservation* fallback = nullptr;
    for (std::size_t i = 0; i < pipelineResult.observationCount; ++i) {
        const auto& observation = pipelineResult.observations[i];
        if (!observation.valid || !observation.pointerActive) continue;
        if (observation.handedness == m_inputConfig.preferredHand)
            return &observation;
        if (observation.handedness == Handedness::LEFT ||
            observation.handedness == Handedness::RIGHT) {
            fallback = &observation;
        }
    }
    return fallback;
}

const GestureEvent* ActionDispatcher::selectVerticalSwipe(
    const GesturePipelineResult& pipelineResult) const {
    const GestureEvent* fallback = nullptr;
    for (std::size_t i = 0; i < pipelineResult.events.count; ++i) {
        const GestureEvent& event = pipelineResult.events.events[i];
        if (event.type != GestureEventType::SWIPE_UP &&
            event.type != GestureEventType::SWIPE_DOWN) {
            continue;
        }
        if (event.handedness == m_inputConfig.preferredHand) return &event;
        if (event.handedness == Handedness::LEFT ||
            event.handedness == Handedness::RIGHT) {
            fallback = &event;
        }
    }
    return fallback;
}

ActionDispatcher::FrameMetadata ActionDispatcher::metadata(
    const GesturePipelineResult& pipelineResult) {
    if (pipelineResult.observationCount > 0) {
        const auto& observation = pipelineResult.observations[0];
        return {true, observation.frameId, observation.timestampUs};
    }
    if (pipelineResult.events.count > 0) {
        const auto& event = pipelineResult.events.events[0];
        return {true, event.frameId, event.timestampUs};
    }
    return {};
}

ActionCommand ActionDispatcher::command(ActionType type,
                                        const FrameMetadata& frame,
                                        Handedness hand,
                                        DesktopPoint point,
                                        int scrollNotches) const {
    return {type, point, hand, frame.frameId, frame.timestampUs,
            scrollNotches};
}

void ActionDispatcher::fail(ActionDispatchResult& result,
                            const std::string& error) {
    result.success = false;
    result.error = error;
    m_lastError = error;
}

bool ActionDispatcher::dispatchButtonDown(const ActionCommand& action,
                                          ActionDispatchResult& result) {
    if (m_buttonDown) return true;
    if (!m_backend.primaryButtonDown()) {
        fail(result, m_backend.lastError());
        return false;
    }
    m_buttonDown = true;
    m_buttonHand = action.handedness;
    result.record(action);
    return true;
}

bool ActionDispatcher::dispatchButtonUp(const ActionCommand& action,
                                        ActionDispatchResult& result) {
    if (!m_buttonDown) return true;
    if (!m_backend.primaryButtonUp()) {
        fail(result, m_backend.lastError());
        return false;
    }
    m_buttonDown = false;
    m_buttonHand = Handedness::UNKNOWN;
    result.record(action);
    return true;
}

bool ActionDispatcher::dispatchMove(const ActionCommand& action,
                                    ActionDispatchResult& result) {
    if (m_backend.movePointer(action.desktopPoint)) {
        result.record(action);
        return true;
    }

    const std::string moveError = m_backend.lastError();
    // One best-effort recovery only. Keep ownership if UP also fails so a
    // later disable/shutdown can retry; never spin in the hot path.
    if (m_buttonDown && m_backend.primaryButtonUp()) {
        m_buttonDown = false;
        m_buttonHand = Handedness::UNKNOWN;
        result.record(command(ActionType::PRIMARY_BUTTON_UP,
                              {true, action.frameId, action.timestampUs},
                              action.handedness));
    }
    fail(result, moveError);
    return false;
}

bool ActionDispatcher::dispatchScroll(const ActionCommand& action,
                                      ActionDispatchResult& result) {
    if (!m_backend.scrollVertical(action.scrollNotches)) {
        fail(result, m_backend.lastError());
        return false;
    }
    result.record(action);
    return true;
}

ActionDispatchResult ActionDispatcher::process(
    const GesturePipelineResult& pipelineResult) {
    ActionDispatchResult result;
    if (!m_initialized || !m_inputEnabled) return result;

    const FrameMetadata frame = metadata(pipelineResult);
    if (!frame.present || frame.timestampUs < 0 ||
        (m_hasTimestamp && frame.timestampUs <= m_lastTimestampUs)) {
        return result;
    }
    m_lastTimestampUs = frame.timestampUs;
    m_hasTimestamp = true;

    const bool buttonWasDown = m_buttonDown;
    const GestureObservation* selected =
        selectActiveObservation(pipelineResult);
    const Handedness selectedHand = selected == nullptr
        ? Handedness::UNKNOWN : selected->handedness;

    if (selectedHand != m_activeHand) {
        if (m_buttonDown) {
            if (!dispatchButtonUp(command(ActionType::PRIMARY_BUTTON_UP,
                                          frame, m_buttonHand), result)) {
                return result;
            }
        }
        m_activeHand = selectedHand;
    }

    if (m_activeHand != Handedness::UNKNOWN) {
        for (std::size_t i = 0; i < pipelineResult.events.count; ++i) {
            const GestureEvent& event = pipelineResult.events.events[i];
            if (event.handedness != m_activeHand) continue;
            if (event.type == GestureEventType::PINCH_BEGIN) {
                if (!dispatchButtonDown(command(
                        ActionType::PRIMARY_BUTTON_DOWN,
                        {true, event.frameId, event.timestampUs},
                        m_activeHand), result)) {
                    return result;
                }
            } else if (event.type == GestureEventType::PINCH_END ||
                       event.type == GestureEventType::PINCH_CANCEL) {
                if (!dispatchButtonUp(command(
                        ActionType::PRIMARY_BUTTON_UP,
                        {true, event.frameId, event.timestampUs},
                        m_activeHand), result)) {
                    return result;
                }
            }
        }
    }

    const GestureEvent* swipe = selectVerticalSwipe(pipelineResult);
    if (swipe != nullptr && m_inputConfig.swipeScrollEnabled &&
        !buttonWasDown && !m_buttonDown) {
        int notches = m_inputConfig.scrollNotchesPerSwipe;
        if (swipe->type == GestureEventType::SWIPE_DOWN) notches = -notches;
        if (m_inputConfig.invertSwipeScroll) notches = -notches;
        if (!dispatchScroll(command(
                ActionType::SCROLL_VERTICAL,
                {true, swipe->frameId, swipe->timestampUs},
                swipe->handedness, {}, notches), result)) {
            return result;
        }
    }

    if (selected != nullptr) {
        const auto mapped = m_pointerMapper.map(selected->pointerPoint, m_desktop);
        if (!mapped.has_value()) {
            fail(result, "pointer mapping failed");
            if (m_buttonDown) releaseAll();
            return result;
        }
        dispatchMove(command(ActionType::MOVE_POINTER, frame, m_activeHand,
                             *mapped), result);
    }
    return result;
}

bool ActionDispatcher::releaseAll() {
    if (!m_buttonDown) return true;
    if (!m_backend.primaryButtonUp()) {
        m_lastError = m_backend.lastError();
        return false;
    }
    m_buttonDown = false;
    m_buttonHand = Handedness::UNKNOWN;
    return true;
}

bool ActionDispatcher::resetRuntimeState() {
    if (!releaseAll()) return false;
    m_activeHand = Handedness::UNKNOWN;
    m_buttonHand = Handedness::UNKNOWN;
    m_lastTimestampUs = 0;
    m_hasTimestamp = false;
    m_lastError.clear();
    return true;
}

bool ActionDispatcher::setInputEnabled(bool enabled) {
    if (enabled == m_inputEnabled) {
        // A prior disable may have failed to release an owned button. Allow a
        // later safety boundary to retry the release.
        if (!enabled && m_buttonDown) return releaseAll();
        return true;
    }
    if (!enabled) {
        const bool released = releaseAll();
        m_inputEnabled = false;
        m_inputConfig.enabled = false;
        m_activeHand = Handedness::UNKNOWN;
        return released;
    }

    if (!m_initialized) {
        m_lastError = "action dispatcher is not initialized";
        return false;
    }
    if (!releaseAll() || !ensureBackendInitialized()) {
        m_inputEnabled = false;
        m_inputConfig.enabled = false;
        return false;
    }
    m_activeHand = Handedness::UNKNOWN;
    m_hasTimestamp = false;
    m_inputEnabled = true;
    m_inputConfig.enabled = true;
    return true;
}

bool ActionDispatcher::applyConfiguration(
    PointerMappingConfig mappingConfig, InputConfig inputConfig) {
    mappingConfig = sanitizePointerMappingConfig(mappingConfig);
    inputConfig = sanitizeInputConfig(inputConfig);
    if (mappingConfig == m_pointerMapper.config() &&
        inputConfig == m_inputConfig) {
        return true;
    }

    if (!releaseAll()) return false;
    if (inputConfig.enabled &&
        (!m_initialized || !ensureBackendInitialized())) {
        m_inputEnabled = false;
        m_inputConfig.enabled = false;
        return false;
    }
    m_pointerMapper = PointerMapper(mappingConfig);
    m_inputConfig = inputConfig;
    m_inputEnabled = inputConfig.enabled;
    m_activeHand = Handedness::UNKNOWN;
    m_hasTimestamp = false;
    m_lastError.clear();
    return true;
}

void ActionDispatcher::shutdown() {
    if (!m_initialized) return;
    releaseAll();
    if (m_backendInitialized) m_backend.shutdown();
    m_initialized = false;
    m_backendInitialized = false;
    m_inputEnabled = false;
    m_activeHand = Handedness::UNKNOWN;
}
