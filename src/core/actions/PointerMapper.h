#ifndef POINTERMAPPER_H
#define POINTERMAPPER_H

#include <optional>

#include "src/core/actions/ActionTypes.h"
#include "src/core/gestures/Landmarks.h"

struct PointerMappingConfig {
    bool mirrorX{false};
    bool mirrorY{false};
    double leftMargin{0.0};
    double rightMargin{0.0};
    double topMargin{0.0};
    double bottomMargin{0.0};

    bool operator==(const PointerMappingConfig&) const = default;
};

PointerMappingConfig sanitizePointerMappingConfig(
    PointerMappingConfig config);

class PointerMapper {
public:
    explicit PointerMapper(PointerMappingConfig config = {});

    std::optional<DesktopPoint> map(
        const Point3D& normalizedCameraPoint,
        const DesktopGeometry& desktop) const;

    const PointerMappingConfig& config() const { return m_config; }

private:
    static double mapAxis(double value, double lowMargin, double highMargin,
                          bool mirror);

    PointerMappingConfig m_config;
};

#endif // POINTERMAPPER_H
