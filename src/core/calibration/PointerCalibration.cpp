#include "PointerCalibration.h"

#include <algorithm>
#include <cmath>

PointerCalibration::PointerCalibration(PointerMappingConfig previous,
                                       std::size_t minimumSamplesPerCorner,
                                       double minimumSpan)
    : m_previous(sanitizePointerMappingConfig(previous)),
      m_minimumSamples(std::max<std::size_t>(3, minimumSamplesPerCorner)),
      m_minimumSpan(std::isfinite(minimumSpan) && minimumSpan > 0.0 &&
                            minimumSpan < 1.0
                        ? minimumSpan
                        : 0.10) {}

bool PointerCalibration::addSample(CalibrationCorner corner,
                                   const Point3D& sample) {
    if (!std::isfinite(sample.x) || !std::isfinite(sample.y) ||
        !std::isfinite(sample.z) ||
        sample.x < 0.0 || sample.x > 1.0 || sample.y < 0.0 ||
        sample.y > 1.0) {
        return false;
    }
    auto& samples = corner == CalibrationCorner::TOP_LEFT
        ? m_topLeft : m_bottomRight;
    samples.push_back(sample);
    return true;
}

std::size_t PointerCalibration::sampleCount(CalibrationCorner corner) const {
    return corner == CalibrationCorner::TOP_LEFT
        ? m_topLeft.size() : m_bottomRight.size();
}

bool PointerCalibration::ready() const {
    return m_topLeft.size() >= m_minimumSamples &&
           m_bottomRight.size() >= m_minimumSamples;
}

double PointerCalibration::median(std::vector<double> values) {
    const auto middle = values.begin() +
        static_cast<std::ptrdiff_t>(values.size() / 2);
    std::nth_element(values.begin(), middle, values.end());
    if (values.size() % 2 != 0) return *middle;
    const double high = *middle;
    const double low = *std::max_element(values.begin(), middle);
    return (low + high) / 2.0;
}

std::optional<PointerMappingConfig> PointerCalibration::result() const {
    if (!ready()) return std::nullopt;

    std::vector<double> leftX;
    std::vector<double> topY;
    std::vector<double> rightX;
    std::vector<double> bottomY;
    leftX.reserve(m_topLeft.size());
    topY.reserve(m_topLeft.size());
    rightX.reserve(m_bottomRight.size());
    bottomY.reserve(m_bottomRight.size());
    for (const auto& sample : m_topLeft) {
        leftX.push_back(sample.x);
        topY.push_back(sample.y);
    }
    for (const auto& sample : m_bottomRight) {
        rightX.push_back(sample.x);
        bottomY.push_back(sample.y);
    }

    const double left = median(std::move(leftX));
    const double top = median(std::move(topY));
    const double right = median(std::move(rightX));
    const double bottom = median(std::move(bottomY));
    if (right - left < m_minimumSpan || bottom - top < m_minimumSpan)
        return std::nullopt;

    PointerMappingConfig calibrated = m_previous;
    calibrated.leftMargin = left;
    calibrated.rightMargin = 1.0 - right;
    calibrated.topMargin = top;
    calibrated.bottomMargin = 1.0 - bottom;
    calibrated = sanitizePointerMappingConfig(calibrated);
    if (calibrated.leftMargin + calibrated.rightMargin >= 1.0 ||
        calibrated.topMargin + calibrated.bottomMargin >= 1.0) {
        return std::nullopt;
    }
    return calibrated;
}

void PointerCalibration::resetSamples() {
    m_topLeft.clear();
    m_bottomRight.clear();
}
