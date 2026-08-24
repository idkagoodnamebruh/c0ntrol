#include "src/platform/windows/WindowsSystemInputBackend.h"

int main() {
    WindowsSystemInputBackend backend;
    const auto scrollMethod = &WindowsSystemInputBackend::scrollVertical;
    (void)scrollMethod;
    (void)backend.lastError();
    backend.shutdown();
    return 0;
}
