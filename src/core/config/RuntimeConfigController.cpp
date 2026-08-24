#include "RuntimeConfigController.h"

RuntimeConfigController::RuntimeConfigController(
    RuntimeConfig initial, ActionDispatcher& actionDispatcher)
    : m_current(sanitizeRuntimeConfig(initial)),
      m_actionDispatcher(actionDispatcher) {}

RuntimeConfigApplyResult RuntimeConfigController::apply(
    const RuntimeConfig& requested) {
    return applyInternal(requested, false);
}

RuntimeConfigApplyResult RuntimeConfigController::resetToDefaults() {
    return applyInternal(RuntimeConfig{}, true);
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
    result.changes.pointerChanged =
        sanitized.pointer != m_current.pointer;
    result.changes.inputChanged = sanitized.input != m_current.input;

    if ((forceRelease || result.changes.cameraRestartRequired) &&
        !m_actionDispatcher.releaseAll()) {
        result.success = false;
        result.error = m_actionDispatcher.lastError();
        return result;
    }
    if (!m_actionDispatcher.applyConfiguration(sanitized.pointer,
                                               sanitized.input)) {
        result.success = false;
        result.error = m_actionDispatcher.lastError();
        return result;
    }
    m_current = sanitized;
    return result;
}
