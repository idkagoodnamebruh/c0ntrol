#include <iostream>
#include <cassert>
#include "src/core/filters/OneEuroFilter.h"

void testFilterSmoothness() {
    OneEuroFilter filter(30.0, 1.0, 0.007);

    double val1 = filter.filter(10.0, 0.0);
    double val2 = filter.filter(10.1, 0.033);
    double val3 = filter.filter(10.2, 0.066);

    // El filtro debe suavizar las variaciones pequeñas sin saltos bruscos
    assert(std::abs(val2 - val1) < 0.1);
    assert(std::abs(val3 - val2) < 0.1);

    std::cout << "[PASS] testFilterSmoothness" << std::endl;
}

int main() {
    testFilterSmoothness();
    return 0;
}
