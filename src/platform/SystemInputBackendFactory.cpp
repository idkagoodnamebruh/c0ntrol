#include "SystemInputBackendFactory.h"

#ifdef _WIN32
#include "src/platform/windows/WindowsSystemInputBackend.h"
#else
#include "src/platform/NullSystemInputBackend.h"
#endif

std::unique_ptr<ISystemInputBackend> createSystemInputBackend() {
#ifdef _WIN32
    return std::make_unique<WindowsSystemInputBackend>();
#else
    return std::make_unique<NullSystemInputBackend>();
#endif
}
