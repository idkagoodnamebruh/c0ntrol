#include "PointerMapper.h"

#include <algorithm>
#include <cmath>

namespace {

double validMargin(double value, double fallback) {
    return std::isfinite(value) && value >= 0.0 && value < 1.0
        ? value : fallback;
}

} // namespace

PointerMappingConfig sanitizePointerMappingConfig(
    PointerMappingConfig config) {
    const PointerMappingConfig defaults;
    config.leftMargin = validMargin(config.leftMargin, defaults.leftMargin);
    config.rightMargin = validMargin(config.rightMargin, defaults.rightMargin);
    config.topMargin = validMargin(config.topMargin, defaults.topMargin);
    config.bottomMargin = validMargin(config.bottomMargin,
                                      defaults.bottomMargin);
    if (config.leftMargin + config.rightMargin >= 1.0) {
        config.leftMargin = defaults.leftMargin;
        config.rightMargin = defaults.rightMargin;
    }
    if (config.topMargin + config.bottomMargin >= 1.0) {
        config.topMargin = defaults.topMargin;
        config.bottomMargin = defaults.bottomMargin;
    }
    return config;
}

PointerMapper::PointerMapper(PointerMappingConfig config)
    : m_config(sanitizePointerMappingConfig(config)) {}

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
