#include <iostream>

#include "src/platform/linux/LinuxEisSystemInputBackend.h"
#include "src/platform/SystemInputBackendFactory.h"

int main() {
    // Construction/destruction verifies real headers and linkage without
    // calling initialize(), so CI never opens RemoteDesktop or sends input.
    std::unique_ptr<ISystemInputBackend> backend = createSystemInputBackend();
    if (dynamic_cast<LinuxEisSystemInputBackend*>(backend.get()) == nullptr)
        return 1;
    backend->shutdown();
    backend->shutdown();
    std::cout << "[PASS] test_linux_eis_backend_compile\n";
    return 0;
}
