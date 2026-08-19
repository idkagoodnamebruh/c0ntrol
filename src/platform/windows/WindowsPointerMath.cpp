#include "WindowsPointerMath.h"

#include <algorithm>
#include <cstdint>

namespace {

long normalizeAxis(int pixel, int origin, int length) {
    if (length <= 1) return 0;
    const int clamped = std::clamp(pixel, origin, origin + length - 1);
    const std::int64_t offset = static_cast<std::int64_t>(clamped) - origin;
    return static_cast<long>((offset * 65'535 + (length - 1) / 2) /
                             (length - 1));
}

} // namespace

std::optional<SendInputAbsolutePoint> desktopPixelToSendInputAbsolute(
    const DesktopPoint& point, const DesktopGeometry& virtualDesktop) {
    if (!virtualDesktop.isValid()) return std::nullopt;
    return SendInputAbsolutePoint{
        normalizeAxis(point.x, virtualDesktop.originX, virtualDesktop.width),
        normalizeAxis(point.y, virtualDesktop.originY, virtualDesktop.height),
    };
}
