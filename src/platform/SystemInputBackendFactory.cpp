#include "SystemInputBackendFactory.h"

#ifdef _WIN32
#include "src/platform/windows/WindowsSystemInputBackend.h"
#elif defined(C0NTROL_HAS_LIBEI)
#include "src/platform/linux/LinuxEisSystemInputBackend.h"
#else
#include "src/platform/NullSystemInputBackend.h"
#endif

std::unique_ptr<ISystemInputBackend> createSystemInputBackend() {
#ifdef _WIN32
    return std::make_unique<WindowsSystemInputBackend>();
#elif defined(C0NTROL_HAS_LIBEI)
    return std::make_unique<LinuxEisSystemInputBackend>(
        createLibeiPortalSession());
#else
    return std::make_unique<NullSystemInputBackend>();
#endif
}
