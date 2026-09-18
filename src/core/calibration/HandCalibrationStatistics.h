#ifndef HANDCALIBRATIONSTATISTICS_H
#define HANDCALIBRATIONSTATISTICS_H

#include <optional>
#include <vector>

#include "HandCalibrationTypes.h"

namespace hand_calibration {

// Sorts scratch storage in place. Rejects any non-finite input (no silent filtering).
// Empty input has an available summary with sampleCount == 0.
// Quantiles use linear interpolation at h = (n - 1) * p (Hyndman-Fan type 7).
std::optional<HandCalibrationStatistics> summarize(std::vector<double>& values);

} // namespace hand_calibration

#endif // HANDCALIBRATIONSTATISTICS_H
