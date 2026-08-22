#include "src/platform/windows/WindowsSystemInputBackend.h"

int main() {
    WindowsSystemInputBackend backend;
    (void)backend.lastError();
    backend.shutdown();
    return 0;
}
