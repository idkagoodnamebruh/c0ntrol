#include <iostream>
#include <cassert>
#include "src/core/vision/FrameSynchronizer.h"

void testSyncDuration() {
    FrameSynchronizer sync(60); // 60 FPS (~16ms por frame)
    
    auto start = std::chrono::steady_clock::now();
    sync.sync();
    sync.sync();
    auto end = std::chrono::steady_clock::now();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    assert(elapsedMs >= 15);

    std::cout << "[PASS] testSyncDuration" << std::endl;
}

int main() {
    testSyncDuration();
    return 0;
}
