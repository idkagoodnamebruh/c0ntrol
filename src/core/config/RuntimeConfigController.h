#ifndef RUNTIMECONFIGCONTROLLER_H
#define RUNTIMECONFIGCONTROLLER_H

#include <string>

#include "src/core/actions/ActionDispatcher.h"
#include "src/core/config/RuntimeConfig.h"

struct RuntimeConfigChanges {
    bool cameraRestartRequired{false};
    bool filteringChanged{false};
    bool gesturesChanged{false};
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
    RuntimeConfigController(RuntimeConfig initial,
                            ActionDispatcher& actionDispatcher);

    RuntimeConfigApplyResult apply(const RuntimeConfig& requested);
    RuntimeConfigApplyResult resetToDefaults();
    const RuntimeConfig& current() const { return m_current; }

private:
    RuntimeConfigApplyResult applyInternal(const RuntimeConfig& requested,
                                           bool forceRelease);

    RuntimeConfig m_current;
    ActionDispatcher& m_actionDispatcher;
};

#endif // RUNTIMECONFIGCONTROLLER_H
