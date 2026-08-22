#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "src/core/actions/PointerMapper.h"

namespace {
void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}
}

int main() {
    const DesktopGeometry desktop{0, 0, 1920, 1080};
    PointerMapper mapper;
    auto point = mapper.map({0.5, 0.5, 0.0}, desktop);
    require(point && point->x == 960 && point->y == 540, "center mapping");
    point = mapper.map({0.0, 0.0, 0.0}, desktop);
    require(point && point->x == 0 && point->y == 0, "top-left corner");
    point = mapper.map({1.0, 1.0, 0.0}, desktop);
    require(point && point->x == 1919 && point->y == 1079,
            "bottom-right corner");
    point = mapper.map({-5.0, 4.0, 0.0}, desktop);
    require(point && point->x == 0 && point->y == 1079,
            "out-of-range values clamp");

    PointerMapper mirrorX({true, false});
    point = mirrorX.map({0.25, 0.3, 0.0}, desktop);
    require(point && point->x == 1439, "mirror X");
    PointerMapper mirrorY({false, true});
    point = mirrorY.map({0.25, 0.25, 0.0}, desktop);
    require(point && point->y == 809, "mirror Y");

    PointerMappingConfig activeConfig;
    activeConfig.leftMargin = 0.2;
    activeConfig.rightMargin = 0.2;
    activeConfig.topMargin = 0.1;
    activeConfig.bottomMargin = 0.1;
    PointerMapper active(activeConfig);
    auto low = active.map({0.2, 0.1, 0.0}, desktop);
    auto high = active.map({0.8, 0.9, 0.0}, desktop);
    require(low && low->x == 0 && low->y == 0, "active-region low edge");
    require(high && high->x == 1919 && high->y == 1079,
            "active-region high edge");

    const DesktopGeometry virtualDesktop{-1920, -200, 3840, 2160};
    point = mapper.map({0.5, 0.5, 0.0}, virtualDesktop);
    require(point && point->x == 0 && point->y == 880,
            "negative virtual desktop origin");
    point = mapper.map({0.0, 0.0, 0.0}, {100, 50, 800, 600});
    require(point && point->x == 100 && point->y == 50,
            "non-zero desktop origin");
    require(!mapper.map({0.5, 0.5, 0.0}, {}).has_value(),
            "invalid desktop geometry");
    require(!mapper.map({std::numeric_limits<double>::quiet_NaN(), 0.5, 0.0},
                        desktop).has_value(),
            "non-finite camera coordinate");

    std::cout << "[PASS] test_pointer_mapper\n";
    return 0;
}
