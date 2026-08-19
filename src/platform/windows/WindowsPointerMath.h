#ifndef WINDOWSPOINTERMATH_H
#define WINDOWSPOINTERMATH_H

#include <optional>

#include "src/core/actions/ActionTypes.h"

struct SendInputAbsolutePoint {
    long x{0};
    long y{0};
};

std::optional<SendInputAbsolutePoint> desktopPixelToSendInputAbsolute(
    const DesktopPoint& point, const DesktopGeometry& virtualDesktop);

#endif // WINDOWSPOINTERMATH_H
