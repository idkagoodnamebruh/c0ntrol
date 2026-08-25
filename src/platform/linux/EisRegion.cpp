#include "EisRegion.h"

#include <algorithm>
#include <limits>

namespace {

bool checkedEnd(std::int64_t start, std::int64_t size, std::int64_t& end) {
    if (size <= 0 || start > std::numeric_limits<std::int64_t>::max() - size)
        return false;
    end = start + size;
    return true;
}

bool fitsInt(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() &&
           value <= std::numeric_limits<int>::max();
}

} // namespace

bool EisRegion::isValid() const {
    std::int64_t endX = 0;
    std::int64_t endY = 0;
    return checkedEnd(x, width, endX) && checkedEnd(y, height, endY);
}

bool EisRegion::contains(const DesktopPoint& point) const {
    std::int64_t endX = 0;
    std::int64_t endY = 0;
    if (!checkedEnd(x, width, endX) || !checkedEnd(y, height, endY))
        return false;
    return point.x >= x && point.x < endX &&
           point.y >= y && point.y < endY;
}

bool pointInEisRegions(const DesktopPoint& point,
                       const std::vector<EisRegion>& regions) {
    return std::any_of(regions.begin(), regions.end(),
                       [&point](const EisRegion& region) {
                           return region.contains(point);
                       });
}

std::optional<DesktopGeometry> eisDesktopGeometry(
    const std::vector<EisRegion>& regions) {
    if (regions.empty()) return std::nullopt;

    std::int64_t minX = std::numeric_limits<std::int64_t>::max();
    std::int64_t minY = std::numeric_limits<std::int64_t>::max();
    std::int64_t maxX = std::numeric_limits<std::int64_t>::min();
    std::int64_t maxY = std::numeric_limits<std::int64_t>::min();
    for (const EisRegion& region : regions) {
        std::int64_t endX = 0;
        std::int64_t endY = 0;
        if (!checkedEnd(region.x, region.width, endX) ||
            !checkedEnd(region.y, region.height, endY)) {
            return std::nullopt;
        }
        minX = std::min(minX, region.x);
        minY = std::min(minY, region.y);
        maxX = std::max(maxX, endX);
        maxY = std::max(maxY, endY);
    }

    if ((minX < 0 && maxX >
             std::numeric_limits<std::int64_t>::max() + minX) ||
        (minY < 0 && maxY >
             std::numeric_limits<std::int64_t>::max() + minY)) {
        return std::nullopt;
    }
    const std::int64_t width = maxX - minX;
    const std::int64_t height = maxY - minY;
    if (!fitsInt(minX) || !fitsInt(minY) || !fitsInt(width) ||
        !fitsInt(height) || width <= 0 || height <= 0) {
        return std::nullopt;
    }
    return DesktopGeometry{static_cast<int>(minX), static_cast<int>(minY),
                           static_cast<int>(width), static_cast<int>(height)};
}

std::optional<std::int32_t> eisDiscreteVerticalScroll(int logicalNotches) {
    if (logicalNotches == 0) return std::nullopt;
    // The core follows Windows wheel semantics (positive is wheel-forward /
    // up). libei's vertical axis follows screen coordinates, so invert once
    // at this platform boundary and use 120 units per logical wheel click.
    const std::int64_t value = -static_cast<std::int64_t>(logicalNotches) * 120;
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::int32_t>(value);
}
