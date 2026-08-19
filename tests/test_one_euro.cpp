#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "src/core/filters/OneEuroFilter.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

double variance(const std::vector<double>& values) {
    double mean = 0.0;
    for (double value : values) mean += value;
    mean /= static_cast<double>(values.size());
    double sum = 0.0;
    for (double value : values) {
        const double delta = value - mean;
        sum += delta * delta;
    }
    return sum / static_cast<double>(values.size());
}

void testFirstAndConstantSamples() {
    OneEuroFilter filter;
    const double first = filter.filter(0.42, 1.0);
    require(std::isfinite(first) && first == 0.42, "first sample is stable");
    require(filter.initialized(), "first valid sample initializes filter");
    for (int i = 1; i <= 30; ++i) {
        const double output = filter.filter(0.42, 1.0 + i / 60.0);
        require(std::abs(output - 0.42) < 1e-12, "constant signal remains constant");
    }
}

void testJitterReduction() {
    OneEuroFilter filter({1.0, 0.0, 1.0, 1.0});
    const double offsets[] = {-0.012, 0.009, -0.006, 0.011, -0.008, 0.004,
                              -0.010, 0.007, -0.003, 0.012, -0.009, 0.005};
    std::vector<double> raw;
    std::vector<double> filtered;
    for (int i = 0; i < 120; ++i) {
        const double sample = 0.5 + offsets[i % 12];
        raw.push_back(sample);
        filtered.push_back(filter.filter(sample, i / 60.0));
    }
    require(variance(filtered) < variance(raw), "filtered jitter variance is lower");
}

void testStepAndAdaptiveBeta() {
    OneEuroFilter stepFilter({1.0, 0.0, 1.0, 1.0});
    stepFilter.filter(0.0, 0.0);
    const double firstStep = stepFilter.filter(1.0, 0.02);
    const double secondStep = stepFilter.filter(1.0, 0.04);
    require(firstStep > 0.0 && firstStep < 1.0, "step response is progressive");
    require(secondStep > firstStep && secondStep < 1.0, "step response converges");

    OneEuroFilter lowBeta({1.0, 0.0, 1.0, 1.0});
    OneEuroFilter highBeta({1.0, 2.0, 1.0, 1.0});
    lowBeta.filter(0.0, 0.0);
    highBeta.filter(0.0, 0.0);
    const double slow = lowBeta.filter(1.0, 0.02);
    const double fast = highBeta.filter(1.0, 0.02);
    require(fast > slow, "higher beta responds faster to motion");
}

void testDerivativeCutoffAndNonUniformTime() {
    OneEuroFilter lowDerivativeCutoff({1.0, 1.0, 0.1, 1.0});
    OneEuroFilter highDerivativeCutoff({1.0, 1.0, 10.0, 1.0});
    lowDerivativeCutoff.filter(0.0, 0.0);
    highDerivativeCutoff.filter(0.0, 0.0);
    const double lowResponse = lowDerivativeCutoff.filter(1.0, 0.02);
    const double highResponse = highDerivativeCutoff.filter(1.0, 0.02);
    require(highResponse > lowResponse, "dCutoff changes derivative adaptation");

    OneEuroFilter irregular;
    const double times[] = {0.0, 0.007, 0.031, 0.094, 0.101, 0.25};
    for (int i = 0; i < 6; ++i) {
        const double output = irregular.filter(0.1 * i, times[i]);
        require(std::isfinite(output), "non-uniform timestamps remain finite");
    }
}

void testInvalidTimestampsAndReset() {
    OneEuroFilter repeated;
    repeated.filter(0.0, 0.0);
    repeated.filter(1.0, 0.1);
    require(repeated.filter(3.0, 0.1) == 3.0,
            "repeated timestamp resets to raw sample");

    OneEuroFilter regressive;
    regressive.filter(0.0, 1.0);
    require(regressive.filter(2.0, 0.5) == 2.0,
            "regressive timestamp resets to raw sample");

    OneEuroFilter largeGap;
    largeGap.filter(0.0, 0.0);
    require(largeGap.filter(1.0, 2.0) == 1.0,
            "extremely large dt resets to raw sample");

    OneEuroFilter invalidTime;
    invalidTime.filter(0.0, 0.0);
    require(invalidTime.filter(0.75, std::numeric_limits<double>::quiet_NaN()) == 0.75,
            "non-finite timestamp passes through raw sample");
    require(!invalidTime.initialized(), "non-finite timestamp leaves clean state");

    repeated.reset();
    require(!repeated.initialized(), "reset clears initialized state");
    require(repeated.filter(-0.25, 5.0) == -0.25,
            "first sample after reset is raw");
}

void testNonFiniteSamplesDoNotCorruptState() {
    OneEuroFilter filter;
    filter.filter(0.25, 0.0);
    const double lastValid = filter.filter(0.5, 0.1);
    const double nanResult =
        filter.filter(std::numeric_limits<double>::quiet_NaN(), 0.2);
    const double infResult =
        filter.filter(std::numeric_limits<double>::infinity(), 0.2);
    require(nanResult == lastValid && infResult == lastValid,
            "NaN and Inf return the last valid filtered sample");
    const double recovered = filter.filter(0.75, 0.2);
    require(std::isfinite(recovered), "valid sample recovers after NaN and Inf");

    OneEuroFilter fresh;
    require(fresh.filter(std::numeric_limits<double>::quiet_NaN(), 0.0) == 0.0,
            "invalid first sample returns finite neutral value");
    require(!fresh.initialized(), "invalid first sample does not initialize state");
}

} // namespace

int main() {
    testFirstAndConstantSamples();
    testJitterReduction();
    testStepAndAdaptiveBeta();
    testDerivativeCutoffAndNonUniformTime();
    testInvalidTimestampsAndReset();
    testNonFiniteSamplesDoNotCorruptState();
    std::cout << "[PASS] test_one_euro\n";
    return 0;
}
