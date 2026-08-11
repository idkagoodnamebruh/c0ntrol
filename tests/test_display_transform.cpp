#include <iostream>
#include <cassert>
#include "src/core/vision/DisplayTransform.h"

void testMapToDisplay() {
    int outX = 0, outY = 0;
    DisplayTransform::mapToDisplay(0.5, 0.5, 1920, 1080, outX, outY);

    assert(outX == 960);
    assert(outY == 540);

    // Test de bordes (clamping)
    DisplayTransform::mapToDisplay(1.5, -0.5, 1920, 1080, outX, outY);
    assert(outX == 1919);
    assert(outY == 0);

    std::cout << "[PASS] testMapToDisplay" << std::endl;
}

int main() {
    testMapToDisplay();
    return 0;
}
