#ifndef EISREGION_H
#define EISREGION_H

#include <cstdint>
#include <optional>
#include <vector>

#include "src/core/actions/ActionTypes.h"

struct EisRegion {
    std::int64_t x{0};
    std::int64_t y{0};
    std::int64_t width{0};
    std::int64_t height{0};

    bool isValid() const;
    bool contains(const DesktopPoint& point) const;
};

bool pointInEisRegions(const DesktopPoint& point,
                       const std::vector<EisRegion>& regions);
std::optional<DesktopGeometry> eisDesktopGeometry(
    const std::vector<EisRegion>& regions);
std::optional<std::int32_t> eisDiscreteVerticalScroll(int logicalNotches);

#endif // EISREGION_H
