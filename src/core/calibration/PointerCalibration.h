#ifndef POINTERCALIBRATION_H
#define POINTERCALIBRATION_H

#include <cstddef>
#include <optional>
#include <vector>

#include "src/core/actions/PointerMapper.h"

enum class CalibrationCorner {
    TOP_LEFT,
    BOTTOM_RIGHT,
};

class PointerCalibration {
public:
    explicit PointerCalibration(PointerMappingConfig previous = {},
                                std::size_t minimumSamplesPerCorner = 5,
                                double minimumSpan = 0.10);

    bool addSample(CalibrationCorner corner, const Point3D& sample);
    std::size_t sampleCount(CalibrationCorner corner) const;
    bool ready() const;
    std::optional<PointerMappingConfig> result() const;
    const PointerMappingConfig& previousConfig() const { return m_previous; }
    void resetSamples();

private:
    static double median(std::vector<double> values);

    PointerMappingConfig m_previous;
    std::size_t m_minimumSamples;
    double m_minimumSpan;
    std::vector<Point3D> m_topLeft;
    std::vector<Point3D> m_bottomRight;
};

#endif // POINTERCALIBRATION_H
