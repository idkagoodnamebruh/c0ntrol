#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

#include "src/core/calibration/PointerCalibration.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

void add(PointerCalibration& calibration, CalibrationCorner corner,
         std::initializer_list<Point3D> samples) {
    for (const auto& sample : samples)
        require(calibration.addSample(corner, sample),
                "finite calibration sample accepted");
}

} // namespace

int main() {
    PointerCalibration valid({}, 5);
    add(valid, CalibrationCorner::TOP_LEFT,
        {{0.10, 0.20, 0}, {0.11, 0.19, 0}, {0.09, 0.21, 0},
         {0.10, 0.20, 0}, {0.12, 0.18, 0}});
    add(valid, CalibrationCorner::BOTTOM_RIGHT,
        {{0.90, 0.80, 0}, {0.89, 0.81, 0}, {0.91, 0.79, 0},
         {0.90, 0.80, 0}, {0.88, 0.82, 0}});
    const auto region = valid.result();
    require(region.has_value(), "valid multi-sample region calibrates");
    require(std::abs(region->leftMargin - 0.10) < 1e-9 &&
                std::abs(region->rightMargin - 0.10) < 1e-9 &&
                std::abs(region->topMargin - 0.20) < 1e-9 &&
                std::abs(region->bottomMargin - 0.20) < 1e-9,
            "median samples produce expected margins");

    PointerMappingConfig mirroredPrevious;
    mirroredPrevious.mirrorX = true;
    mirroredPrevious.mirrorY = true;
    PointerCalibration mirrored(mirroredPrevious, 3);
    add(mirrored, CalibrationCorner::TOP_LEFT,
        {{0.1, 0.1, 0}, {0.1, 0.1, 0}, {0.1, 0.1, 0}});
    add(mirrored, CalibrationCorner::BOTTOM_RIGHT,
        {{0.9, 0.9, 0}, {0.9, 0.9, 0}, {0.9, 0.9, 0}});
    require(mirrored.result()->mirrorX && mirrored.result()->mirrorY,
            "calibration preserves mirror policy");

    PointerCalibration outlier({}, 5);
    add(outlier, CalibrationCorner::TOP_LEFT,
        {{0.1, 0.1, 0}, {0.1, 0.1, 0}, {0.1, 0.1, 0},
         {0.11, 0.09, 0}, {0.95, 0.95, 0}});
    add(outlier, CalibrationCorner::BOTTOM_RIGHT,
        {{0.9, 0.9, 0}, {0.9, 0.9, 0}, {0.9, 0.9, 0},
         {0.89, 0.91, 0}, {0.05, 0.05, 0}});
    require(outlier.result().has_value() &&
                std::abs(outlier.result()->leftMargin - 0.1) < 1e-9,
            "median rejects a single corner outlier");

    PointerCalibration degenerate({}, 3);
    add(degenerate, CalibrationCorner::TOP_LEFT,
        {{0.5, 0.5, 0}, {0.5, 0.5, 0}, {0.5, 0.5, 0}});
    add(degenerate, CalibrationCorner::BOTTOM_RIGHT,
        {{0.55, 0.55, 0}, {0.55, 0.55, 0}, {0.55, 0.55, 0}});
    require(!degenerate.result().has_value(),
            "degenerate region is rejected");
    require(degenerate.previousConfig() == PointerMappingConfig{},
            "rejection preserves previous configuration");

    PointerCalibration reversed({}, 3);
    add(reversed, CalibrationCorner::TOP_LEFT,
        {{0.8, 0.8, 0}, {0.8, 0.8, 0}, {0.8, 0.8, 0}});
    add(reversed, CalibrationCorner::BOTTOM_RIGHT,
        {{0.2, 0.2, 0}, {0.2, 0.2, 0}, {0.2, 0.2, 0}});
    require(!reversed.result().has_value(),
            "reversed region is rejected");

    PointerCalibration nonFinite({}, 3);
    require(!nonFinite.addSample(
                CalibrationCorner::TOP_LEFT,
                {std::numeric_limits<double>::quiet_NaN(), 0.2, 0}) &&
                !nonFinite.addSample(
                    CalibrationCorner::BOTTOM_RIGHT,
                    {0.8, std::numeric_limits<double>::infinity(), 0}),
            "NaN and Inf samples are rejected");

    PointerMapper mapper(*region);
    const DesktopGeometry desktop{0, 0, 1000, 500};
    const auto low = mapper.map({0.1, 0.2, 0}, desktop);
    const auto high = mapper.map({0.9, 0.8, 0}, desktop);
    require(low && low->x == 0 && low->y == 0 && high &&
                high->x == 999 && high->y == 499,
            "calibration result drives the unique PointerMapper correctly");

    std::cout << "[PASS] test_pointer_calibration\n";
    return 0;
}
