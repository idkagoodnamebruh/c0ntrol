#include "NullSystemInputBackend.h"

bool NullSystemInputBackend::initialize() {
    m_lastError = "native input is unsupported on this platform";
    return false;
}

bool NullSystemInputBackend::movePointer(const DesktopPoint&) {
    m_lastError = "native pointer movement is unsupported on this platform";
    return false;
}

bool NullSystemInputBackend::primaryButtonDown() {
    m_lastError = "native button input is unsupported on this platform";
    return false;
}

bool NullSystemInputBackend::primaryButtonUp() {
    m_lastError = "native button input is unsupported on this platform";
    return false;
}

void NullSystemInputBackend::shutdown() {}
