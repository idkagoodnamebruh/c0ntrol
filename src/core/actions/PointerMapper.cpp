#include "PointerMapper.h"

#include <algorithm>
#include <cmath>

namespace {

double validMargin(double value) {
    return std::isfinite(value) && value >= 0.0 && value < 1.0
        ? value : 0.0;
}

} // namespace

PointerMappingConfig PointerMapper::sanitize(PointerMappingConfig config) {
    config.leftMargin = validMargin(config.leftMargin);
    config.rightMargin = validMargin(config.rightMargin);
    config.topMargin = validMargin(config.topMargin);
    config.bottomMargin = validMargin(config.bottomMargin);
    if (config.leftMargin + config.rightMargin >= 1.0) {
        config.leftMargin = 0.0;
        config.rightMargin = 0.0;
    }
    if (config.topMargin + config.bottomMargin >= 1.0) {
        config.topMargin = 0.0;
        config.bottomMargin = 0.0;
    }
    return config;
}

PointerMapper::PointerMapper(PointerMappingConfig config)
    : m_config(sanitize(config)) {}

double PointerMapper::mapAxis(double value, double lowMargin,
                              double highMargin, bool mirror) {
    const double activeRange = 1.0 - lowMargin - highMargin;
    double mapped = (std::clamp(value, 0.0, 1.0) - lowMargin) / activeRange;
    mapped = std::clamp(mapped, 0.0, 1.0);
    return mirror ? 1.0 - mapped : mapped;
}

std::optional<DesktopPoint> PointerMapper::map(
    const Point3D& normalizedCameraPoint,
    const DesktopGeometry& desktop) const {
    if (!desktop.isValid() || !std::isfinite(normalizedCameraPoint.x) ||
        !std::isfinite(normalizedCameraPoint.y)) {
        return std::nullopt;
    }

    const double x = mapAxis(normalizedCameraPoint.x, m_config.leftMargin,
                             m_config.rightMargin, m_config.mirrorX);
    const double y = mapAxis(normalizedCameraPoint.y, m_config.topMargin,
                             m_config.bottomMargin, m_config.mirrorY);
    return DesktopPoint{
        desktop.originX + static_cast<int>(std::lround(
            x * static_cast<double>(desktop.width - 1))),
        desktop.originY + static_cast<int>(std::lround(
            y * static_cast<double>(desktop.height - 1))),
    };
}
