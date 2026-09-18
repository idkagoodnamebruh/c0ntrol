#include "HandCalibrationStatistics.h"

#include <algorithm>
#include <cmath>

namespace hand_calibration {
namespace {

double percentile(const std::vector<double>& sorted, double probability) {
    const double position = static_cast<double>(sorted.size() - 1) * probability;
    const auto lower = static_cast<std::size_t>(position);
    const auto upper = std::min(lower + 1, sorted.size() - 1);
    // lerp avoids overflow for large finite values with opposite signs.
    return std::lerp(sorted[lower], sorted[upper],
                     position - static_cast<double>(lower));
}

} // namespace

std::optional<HandCalibrationStatistics> summarize(std::vector<double>& values) {
    for (double value : values) {
        if (!std::isfinite(value)) return std::nullopt;
    }
    // Canonicalize signed zero so sorting equal values remains deterministic.
    for (double& value : values) {
        if (value == 0.0) value = 0.0;
    }
    std::sort(values.begin(), values.end());
    if (values.empty()) return HandCalibrationStatistics{};
    return HandCalibrationStatistics{values.size(), percentile(values, 0.10),
                                     percentile(values, 0.50),
                                     percentile(values, 0.90)};
}

} // namespace hand_calibration
