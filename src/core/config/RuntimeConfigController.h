#ifndef RUNTIMECONFIGCONTROLLER_H
#define RUNTIMECONFIGCONTROLLER_H

#include <string>

#include "src/core/actions/ActionDispatcher.h"
#include "src/core/config/RuntimeConfig.h"

struct RuntimeConfigChanges {
    bool cameraRestartRequired{false};
    bool filteringChanged{false};
    bool gesturesChanged{false};
    bool dynamicGesturesChanged{false};
    bool pointerChanged{false};
    bool inputChanged{false};
};

struct RuntimeConfigApplyResult {
    bool success{true};
    RuntimeConfigChanges changes{};
    std::string error;
};

class RuntimeConfigController {
public:
    explicit RuntimeConfigController(RuntimeConfig initial);
    RuntimeConfigController(RuntimeConfig initial,
                            ActionDispatcher& actionDispatcher);

    RuntimeConfigApplyResult apply(const RuntimeConfig& requested);
    RuntimeConfigApplyResult resetToDefaults();
    bool suspendInput(std::string& error);
    bool cancelInputSuspension(std::string& error);
    RuntimeConfigApplyResult completeInputSuspension(
        const RuntimeConfig& requested, bool resetToDefaults);
    bool inputSuspended() const { return m_inputSuspended; }
    const RuntimeConfig& current() const { return m_current; }

private:
    RuntimeConfigApplyResult applyInternal(const RuntimeConfig& requested,
                                           bool forceRelease);

    RuntimeConfig m_current;
    // Optional compatibility hook for synchronous core-only users. The GUI
    // uses the pure constructor and sends effects to NativeInputRuntime.
    ActionDispatcher* m_actionDispatcher{nullptr};
    bool m_inputSuspended{false};
    bool m_restoreInputAfterSuspension{false};
};

#endif // RUNTIMECONFIGCONTROLLER_H
