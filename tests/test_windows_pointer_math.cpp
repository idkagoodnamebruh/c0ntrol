#include <cstdlib>
#include <iostream>

#include "src/platform/windows/WindowsPointerMath.h"

namespace {
void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}
}

int main() {
    const DesktopGeometry primary{0, 0, 1920, 1080};
    auto absolute = desktopPixelToSendInputAbsolute({0, 0}, primary);
    require(absolute && absolute->x == 0 && absolute->y == 0,
            "origin maps to zero");
    absolute = desktopPixelToSendInputAbsolute({1919, 1079}, primary);
    require(absolute && absolute->x == 65'535 && absolute->y == 65'535,
            "bottom-right maps to 65535");

    const DesktopGeometry virtualDesktop{-1920, -1080, 3840, 2160};
    absolute = desktopPixelToSendInputAbsolute({-1920, -1080}, virtualDesktop);
    require(absolute && absolute->x == 0 && absolute->y == 0,
            "negative virtual origin maps to zero");
    absolute = desktopPixelToSendInputAbsolute({1919, 1079}, virtualDesktop);
    require(absolute && absolute->x == 65'535 && absolute->y == 65'535,
            "multi-monitor far edge maps to 65535");
    absolute = desktopPixelToSendInputAbsolute({0, 0}, virtualDesktop);
    const long expectedX = static_cast<long>((1920LL * 65'535 + 1919) / 3839);
    const long expectedY = static_cast<long>((1080LL * 65'535 + 1079) / 2159);
    require(absolute && absolute->x == expectedX && absolute->y == expectedY,
            "desktop zero accounts for negative origin");

    absolute = desktopPixelToSendInputAbsolute({100, 50}, {100, 50, 800, 600});
    require(absolute && absolute->x == 0 && absolute->y == 0,
            "non-zero origin maps correctly");
    require(!desktopPixelToSendInputAbsolute({0, 0}, {}).has_value(),
            "invalid geometry is rejected");
    std::cout << "[PASS] test_windows_pointer_math\n";
    return 0;
}
