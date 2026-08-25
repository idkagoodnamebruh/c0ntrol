#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "src/platform/linux/EisRegion.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

void testSingleAndAdjacentRegions() {
    const std::vector<EisRegion> single{{0, 0, 1920, 1080}};
    const auto geometry = eisDesktopGeometry(single);
    require(geometry && geometry->originX == 0 && geometry->originY == 0 &&
                geometry->width == 1920 && geometry->height == 1080,
            "single region becomes desktop geometry");
    require(pointInEisRegions({0, 0}, single) &&
                pointInEisRegions({1919, 1079}, single) &&
                !pointInEisRegions({1920, 1080}, single),
            "region containment uses half-open edges");

    const std::vector<EisRegion> adjacent{{-1280, 0, 1280, 1024},
                                          {0, 0, 1920, 1080}};
    const auto unionGeometry = eisDesktopGeometry(adjacent);
    require(unionGeometry && unionGeometry->originX == -1280 &&
                unionGeometry->originY == 0 && unionGeometry->width == 3200 &&
                unionGeometry->height == 1080,
            "adjacent regions form an exact bounding union");
}

void testSeparatedRegionsAndInvalidInput() {
    const std::vector<EisRegion> separated{{0, 0, 100, 100},
                                           {200, 0, 100, 100}};
    const auto geometry = eisDesktopGeometry(separated);
    require(geometry && geometry->width == 300 && geometry->height == 100,
            "separated regions retain their bounding desktop");
    require(pointInEisRegions({50, 50}, separated) &&
                pointInEisRegions({250, 50}, separated) &&
                !pointInEisRegions({150, 50}, separated),
            "points in monitor gaps are rejected");
    require(!eisDesktopGeometry({}).has_value(),
            "empty region list is invalid");
    require(!eisDesktopGeometry({{0, 0, 0, 100}}).has_value() &&
                !eisDesktopGeometry({{0, 0, 100, -1}}).has_value(),
            "zero and negative regions are invalid");
}

void testOverflowSafety() {
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    require(!eisDesktopGeometry({{maximum - 5, 0, 10, 10}}).has_value(),
            "region endpoint overflow is rejected");
    require(!eisDesktopGeometry({
                {std::numeric_limits<std::int64_t>::min(), 0, 1, 1},
                {maximum - 1, 0, 1, 1}}).has_value(),
            "bounding-union overflow is rejected");
    require(!eisDesktopGeometry({
                {static_cast<std::int64_t>(std::numeric_limits<int>::max()) + 1,
                 0, 1, 1}}).has_value(),
            "geometry outside the core integer contract is rejected");
}

void testScrollUnitsAndSign() {
    require(eisDiscreteVerticalScroll(1) == -120 &&
                eisDiscreteVerticalScroll(3) == -360 &&
                eisDiscreteVerticalScroll(-2) == 240,
            "logical Windows-compatible wheel direction maps to libei units");
    require(!eisDiscreteVerticalScroll(0).has_value() &&
                !eisDiscreteVerticalScroll(
                    std::numeric_limits<int>::max()).has_value(),
            "zero and overflowing libei scroll values are rejected");
}

} // namespace

int main() {
    testSingleAndAdjacentRegions();
    testSeparatedRegionsAndInvalidInput();
    testOverflowSafety();
    testScrollUnitsAndSign();
    std::cout << "[PASS] test_eis_regions\n";
    return 0;
}
