#include <cassert>
#include <iostream>

#include "src/core/actions/PointerMapper.h"

int main() {
    PointerMapper mapper;
    const DesktopGeometry display{0, 0, 1920, 1080};
    const auto center = mapper.map({0.5, 0.5, 0.0}, display);
    assert(center.has_value() && center->x == 960 && center->y == 540);
    const auto clamped = mapper.map({1.5, -0.5, 0.0}, display);
    assert(clamped.has_value() && clamped->x == 1919 && clamped->y == 0);
    std::cout << "[PASS] test_display_transform (PointerMapper)\n";
    return 0;
}
