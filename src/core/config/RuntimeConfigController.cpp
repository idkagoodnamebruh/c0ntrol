#include "RuntimeConfigController.h"

RuntimeConfigController::RuntimeConfigController(RuntimeConfig initial)
    : m_current(sanitizeRuntimeConfig(initial)) {}

RuntimeConfigController::RuntimeConfigController(
    RuntimeConfig initial, ActionDispatcher& actionDispatcher)
    : m_current(sanitizeRuntimeConfig(initial)),
      m_actionDispatcher(&actionDispatcher) {}

RuntimeConfigApplyResult RuntimeConfigController::apply(
    const RuntimeConfig& requested) {
    return applyInternal(requested, false);
}

RuntimeConfigApplyResult RuntimeConfigController::resetToDefaults() {
    return applyInternal(RuntimeConfig{}, true);
}

bool RuntimeConfigController::suspendInput(std::string& error) {
    error.clear();
    if (m_inputSuspended) return true;
    m_restoreInputAfterSuspension = m_current.input.enabled;
    const bool disabled = m_actionDispatcher == nullptr ||
        m_actionDispatcher->setInputEnabled(false);
    m_inputSuspended = disabled;
    if (!disabled) error = m_actionDispatcher->lastError();
    return disabled;
}

bool RuntimeConfigController::cancelInputSuspension(std::string& error) {
    error.clear();
    if (!m_inputSuspended) return true;
    if (m_actionDispatcher != nullptr &&
        !m_actionDispatcher->setInputEnabled(
            m_restoreInputAfterSuspension)) {
        error = m_actionDispatcher->lastError();
        return false;
    }
    m_inputSuspended = false;
    return true;
}

RuntimeConfigApplyResult RuntimeConfigController::completeInputSuspension(
    const RuntimeConfig& requested, bool resetToDefaults) {
    RuntimeConfigApplyResult result = resetToDefaults
        ? applyInternal(RuntimeConfig{}, true)
        : applyInternal(requested, false);
    if (result.success) m_inputSuspended = false;
    return result;
}

RuntimeConfigApplyResult RuntimeConfigController::applyInternal(
    const RuntimeConfig& requested, bool forceRelease) {
    const RuntimeConfig sanitized = sanitizeRuntimeConfig(requested);
    RuntimeConfigApplyResult result;
    result.changes.cameraRestartRequired =
        sanitized.camera != m_current.camera;
    result.changes.filteringChanged =
        sanitized.filtering != m_current.filtering;
    result.changes.gesturesChanged =
        sanitized.gestures != m_current.gestures;
    result.changes.dynamicGesturesChanged =
        sanitized.dynamicGestures != m_current.dynamicGestures;
    result.changes.pointerChanged =
        sanitized.pointer != m_current.pointer;
    result.changes.inputChanged = sanitized.input != m_current.input;

    if (m_actionDispatcher != nullptr &&
        (forceRelease || result.changes.cameraRestartRequired) &&
        !m_actionDispatcher->releaseAll()) {
        result.success = false;
        result.error = m_actionDispatcher->lastError();
        return result;
    }
    if (m_actionDispatcher != nullptr &&
        !m_actionDispatcher->applyConfiguration(sanitized.pointer,
                                                sanitized.input)) {
        result.success = false;
        result.error = m_actionDispatcher->lastError();
        return result;
    }
    m_current = sanitized;
    return result;
}
