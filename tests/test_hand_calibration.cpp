#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "src/core/calibration/HandCalibrationSession.h"
#include "src/core/calibration/HandCalibrationStatistics.h"

namespace {
using Pose = HandCalibrationPose;
using Result = HandCalibrationSampleResult;
using Status = HandCalibrationProfileStatus;
constexpr auto nan = std::numeric_limits<double>::quiet_NaN();
constexpr auto inf = std::numeric_limits<double>::infinity();

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void near(double actual, double expected) {
    require(std::isfinite(actual) && std::abs(actual - expected) < 1e-12,
            "unexpected numeric result");
}

HandCalibrationObservation observation(Pose pose, std::size_t index = 0) {
    HandCalibrationObservation result;
    result.pose = pose;
    auto& f = result.features;
    const double jitter = static_cast<double>(index % 5) * 0.005;
    f.valid = true;
    f.handedness = Handedness::RIGHT;
    f.handScale = 0.20 + jitter;
    f.pinchRatio = (pose == Pose::PINCH_CLOSED ? 0.12 : 0.70) + jitter;
    f.indexCurl = 0.05 + jitter;
    f.middleCurl = f.ringCurl = f.pinkyCurl =
        (pose == Pose::POINTING ? 0.75 : 0.06) + jitter;
    f.thumbCurl = 0.10 + jitter;
    f.thumbSpreadRatio = 0.90 + jitter;
    return result;
}

std::vector<HandCalibrationObservation> dataset(std::size_t perPose = 10) {
    std::vector<HandCalibrationObservation> result;
    for (auto pose : kHandCalibrationPoses) {
        for (std::size_t i = 0; i < perPose; ++i) {
            auto sample = observation(pose, i);
            sample.normalizedLuma = 0.3 + static_cast<double>(i % 5) * 0.1;
            sample.trackingConfidence = 0.8 + static_cast<double>(i % 5) * 0.02;
            result.push_back(sample);
        }
    }
    return result;
}

HandCalibrationSession sessionFor(const std::vector<HandCalibrationObservation>& data,
                                  HandCalibrationConfig config = {}) {
    HandCalibrationSession session(config);
    for (const auto& sample : data)
        require(session.addObservation(sample) == Result::ACCEPTED, "sample rejected");
    return session;
}

void noRecommendations(const HandCalibrationProfile& p) {
    require(!p.valid && !p.recommendedPinchEnterRatio && !p.recommendedPinchExitRatio &&
                !p.recommendedFingerExtendedMaxCurl && !p.recommendedFingerCurledMinCurl,
            "invalid profile must not expose partial recommendations");
}

void validProfile() {
    auto session = sessionFor(dataset());
    require(session.ready(), "complete acquisition is ready");
    const auto p = session.buildProfile();
    require(p.valid && p.status == Status::VALID && p.version == 1, "valid versioned profile");
    require(p.acceptedSamples == 40 && p.rejectedSamples == 0 &&
                p.handedness == Handedness::RIGHT, "profile metadata");
    require(p.handScale.sampleCount == 40 && p.pinchClosed.sampleCount == 10 &&
                p.pinchOpen.sampleCount == 10, "distribution counts");
    for (auto pose : kHandCalibrationPoses)
        require(session.sampleCount(pose) == 10, "pose count");
}

void insufficientSamples() {
    auto session = sessionFor(dataset(9));
    require(!session.ready(), "below per-pose minimum");
    const auto p = session.buildProfile();
    require(p.status == Status::INSUFFICIENT_SAMPLES && p.acceptedSamples == 36,
            "incomplete profile retains useful diagnostics");
    noRecommendations(p);
    for (auto missing : kHandCalibrationPoses) {
        auto data = dataset(20);
        std::erase_if(data, [missing](const auto& s) { return s.pose == missing; });
        auto incomplete = sessionFor(data);
        require(!incomplete.ready(), "total count cannot replace missing pose");
        noRecommendations(incomplete.buildProfile());
    }
}

void nonFiniteFeatures() {
    const std::array fields{&HandFeatures::handScale, &HandFeatures::pinchRatio,
                            &HandFeatures::indexCurl, &HandFeatures::middleCurl,
                            &HandFeatures::ringCurl, &HandFeatures::pinkyCurl,
                            &HandFeatures::thumbCurl, &HandFeatures::thumbSpreadRatio};
    HandCalibrationSession session;
    std::size_t rejected = 0;
    for (auto pose : kHandCalibrationPoses) {
        for (auto field : fields) {
            for (double bad : {nan, inf, -inf}) {
                auto sample = observation(pose);
                sample.features.*field = bad;
                require(session.addObservation(sample) == Result::INVALID_FEATURES,
                        "all numeric feature fields must reject NaN and infinity");
                ++rejected;
            }
        }
        for (auto point : {&HandFeatures::palmCenter, &HandFeatures::pointerPoint}) {
            for (auto axis : {&Point3D::x, &Point3D::y, &Point3D::z}) {
                for (double bad : {nan, inf, -inf}) {
                    auto sample = observation(pose);
                    (sample.features.*point).*axis = bad;
                    require(session.addObservation(sample) == Result::INVALID_FEATURES,
                            "position metadata must reject non-finite coordinates");
                    ++rejected;
                }
            }
        }
    }
    require(session.acceptedSamples() == 0 && session.rejectedSamples() == rejected,
            "rejection counters");
    require(session.handedness() == Handedness::UNKNOWN, "invalid sample cannot lock hand");
}

void handedness() {
    for (auto initial : {Handedness::LEFT, Handedness::RIGHT}) {
        HandCalibrationSession session;
        auto sample = observation(Pose::OPEN_HAND);
        sample.features.handedness = Handedness::UNKNOWN;
        require(session.addObservation(sample) == Result::INVALID_HANDEDNESS, "unknown hand");
        sample.features.handedness = static_cast<Handedness>(999);
        require(session.addObservation(sample) == Result::INVALID_HANDEDNESS, "invalid hand enum");
        sample.features.handedness = initial;
        require(session.addObservation(sample) == Result::ACCEPTED, "initial hand accepted");
        sample.features.handedness = initial == Handedness::LEFT ? Handedness::RIGHT
                                                               : Handedness::LEFT;
        require(session.addObservation(sample) == Result::OPPOSITE_HAND, "opposite hand rejected");
        require(session.handedness() == initial && session.acceptedSamples() == 1 &&
                    session.rejectedSamples() == 3, "hand lock is stable");
    }
    auto data = dataset();
    for (auto& s : data) s.features.handedness = Handedness::LEFT;
    const auto p = sessionFor(data).buildProfile();
    require(p.valid && p.handedness == Handedness::LEFT, "left-hand profile valid");
}

void percentiles() {
    std::vector<double> values{10, 0, 30, 20};
    const auto stats = hand_calibration::summarize(values);
    require(stats && stats->sampleCount == 4, "statistics count");
    near(stats->p10, 3.0);
    near(stats->median, 15.0);
    near(stats->p90, 27.0);
    values = {9, 1, 5};
    const auto odd = hand_calibration::summarize(values).value();
    near(odd.p10, 1.8); near(odd.median, 5.0); near(odd.p90, 8.2);
}

void statisticEdgeCases() {
    std::vector<double> values;
    require(hand_calibration::summarize(values)->sampleCount == 0, "empty statistics");
    values = {2.5};
    auto stats = hand_calibration::summarize(values).value();
    near(stats.p10, 2.5); near(stats.median, 2.5); near(stats.p90, 2.5);
    values = {0, 10};
    stats = hand_calibration::summarize(values).value();
    near(stats.p10, 1); near(stats.median, 5); near(stats.p90, 9);
    values = {4, 4, 4, 4};
    stats = hand_calibration::summarize(values).value();
    near(stats.p10, 4); near(stats.median, 4); near(stats.p90, 4);
    for (double bad : {nan, inf, -inf}) {
        values = {1, bad, 2};
        require(!hand_calibration::summarize(values), "statistics reject non-finite input");
    }
    const double max = std::numeric_limits<double>::max();
    values = {-max, max};
    stats = hand_calibration::summarize(values).value();
    require(std::isfinite(stats.p10) && std::isfinite(stats.p90), "overflow-safe interpolation");
    near(stats.median, 0);
    values = {-0.0, 0.0, -0.0};
    stats = hand_calibration::summarize(values).value();
    require(!std::signbit(stats.p10) && !std::signbit(stats.median) &&
                !std::signbit(stats.p90), "canonical signed zero");
}

void extremeOutlier() {
    auto data = dataset(20);
    const auto baseline = sessionFor(data).buildProfile();
    auto outlier = observation(Pose::PINCH_CLOSED);
    outlier.features.handScale = 4.0;
    outlier.features.pinchRatio = 10.0;
    outlier.features.thumbSpreadRatio = 10.0;
    outlier.features.thumbCurl = 1.0;
    data.push_back(outlier);
    auto curlOutlier = observation(Pose::OPEN_HAND);
    curlOutlier.features.indexCurl = curlOutlier.features.middleCurl =
        curlOutlier.features.ringCurl = curlOutlier.features.pinkyCurl = 1.0;
    data.push_back(curlOutlier);
    const auto p = sessionFor(data).buildProfile();
    require(p.valid && p.acceptedSamples == 82, "accepted extremes do not destroy calibration");
    require(std::abs(*p.recommendedPinchEnterRatio - *baseline.recommendedPinchEnterRatio) < 0.02,
            "pinch remains robust");
    require(std::abs(*p.recommendedFingerExtendedMaxCurl -
                     *baseline.recommendedFingerExtendedMaxCurl) < 0.02, "curl remains robust");
    require(p.handScale.p90 < 0.3, "scale remains robust");
}

void pinchThresholds() {
    const auto p = sessionFor(dataset()).buildProfile();
    require(p.valid, "pinch fixture valid");
    const double gap = p.pinchOpen.p10 - p.pinchClosed.p90;
    near(*p.recommendedPinchEnterRatio, p.pinchClosed.p90 + gap / 3);
    near(*p.recommendedPinchExitRatio, p.pinchClosed.p90 + 2 * gap / 3);
    require(p.pinchClosed.p90 < *p.recommendedPinchEnterRatio &&
                *p.recommendedPinchEnterRatio < *p.recommendedPinchExitRatio &&
                *p.recommendedPinchExitRatio < p.pinchOpen.p10, "conservative pinch hysteresis");
}

void pinchOverlap() {
    auto data = dataset();
    for (auto& s : data) s.features.pinchRatio = 0.4;
    const auto session = sessionFor(data);
    require(session.ready(), "ready does not promise separability");
    const auto p = session.buildProfile();
    require(p.status == Status::PINCH_NOT_SEPARABLE, "indistinguishable pinch classes");
    noRecommendations(p);
}

void pinchTailOverlap() {
    auto data = dataset();
    std::size_t closed = 0, open = 0;
    for (auto& s : data) {
        if (s.pose == Pose::PINCH_CLOSED) s.features.pinchRatio = closed++ < 3 ? 0.8 : 0.1;
        if (s.pose == Pose::PINCH_OPEN) s.features.pinchRatio = open++ < 3 ? 0.2 : 0.9;
    }
    const auto p = sessionFor(data).buildProfile();
    require(p.pinchOpen.median - p.pinchClosed.median > 0.5, "medians alone would pass");
    require(p.status == Status::PINCH_NOT_SEPARABLE, "overlapping central ranges fail");
    noRecommendations(p);
}

void reversedPinch() {
    auto data = dataset();
    for (auto& s : data) {
        if (s.pose == Pose::PINCH_CLOSED) s.features.pinchRatio = 0.8;
        if (s.pose == Pose::PINCH_OPEN) s.features.pinchRatio = 0.1;
    }
    require(sessionFor(data).buildProfile().status == Status::PINCH_NOT_SEPARABLE,
            "inverted labels fail");
}

void curlThresholds() {
    const auto p = sessionFor(dataset()).buildProfile();
    require(p.valid && p.extendedCurl.sampleCount == 50 && p.curledCurl.sampleCount == 30,
            "semantic pooling counts");
    const double gap = p.curledCurl.p10 - p.extendedCurl.p90;
    near(*p.recommendedFingerExtendedMaxCurl, p.extendedCurl.p90 + gap / 3);
    near(*p.recommendedFingerCurledMinCurl, p.extendedCurl.p90 + 2 * gap / 3);
    require(p.extendedCurl.p90 < *p.recommendedFingerExtendedMaxCurl &&
                *p.recommendedFingerExtendedMaxCurl < *p.recommendedFingerCurledMinCurl &&
                *p.recommendedFingerCurledMinCurl < p.curledCurl.p10, "curl separation");
    require(p.curledCurl.p10 > 0.7, "pointing index never enters curled distribution");
}

void curlOverlap() {
    auto data = dataset();
    for (auto& s : data)
        s.features.indexCurl = s.features.middleCurl = s.features.ringCurl = s.features.pinkyCurl = 0.2;
    const auto p = sessionFor(data).buildProfile();
    require(p.status == Status::CURL_NOT_SEPARABLE, "inseparable curls fail");
    noRecommendations(p);
}

void curlTailOverlap() {
    auto data = dataset();
    std::size_t open = 0, pointing = 0;
    for (auto& s : data) {
        if (s.pose == Pose::OPEN_HAND && open++ < 3)
            s.features.indexCurl = s.features.middleCurl = s.features.ringCurl = s.features.pinkyCurl = 0.9;
        if (s.pose == Pose::POINTING && pointing++ < 3)
            s.features.middleCurl = s.features.ringCurl = s.features.pinkyCurl = 0.1;
    }
    const auto p = sessionFor(data).buildProfile();
    require(p.curledCurl.median - p.extendedCurl.median > 0.5, "curl medians would pass");
    require(p.status == Status::CURL_NOT_SEPARABLE, "curl tails overlap excessively");
    noRecommendations(p);
}

void orderIndependence() {
    auto data = dataset(20);
    const auto baseline = sessionFor(data).buildProfile();
    std::reverse(data.begin(), data.end());
    require(sessionFor(data).buildProfile() == baseline, "reverse order changes profile");
    std::mt19937 generator(417);
    for (int i = 0; i < 20; ++i) {
        std::shuffle(data.begin(), data.end(), generator);
        require(sessionFor(data).buildProfile() == baseline, "shuffle changes profile");
    }
}

void absentLuminance() {
    auto data = dataset();
    for (auto& s : data) { s.normalizedLuma.reset(); s.trackingConfidence.reset(); }
    const auto p = sessionFor(data).buildProfile();
    require(p.valid && p.luminance.sampleCount == 0 && p.trackingConfidence.sampleCount == 0,
            "optional metadata is not required");
}

void invalidLuminance() {
    auto session = sessionFor(dataset());
    const auto before = session.buildProfile();
    for (double bad : {nan, inf, -inf, -0.1, 1.1}) {
        auto sample = observation(Pose::OPEN_HAND);
        sample.normalizedLuma = bad;
        require(session.addObservation(sample) == Result::INVALID_LUMINANCE,
                "invalid luma rejects complete observation");
    }
    const auto p = session.buildProfile();
    require(p.valid && p.luminance == before.luminance && p.acceptedSamples == 40 &&
                p.rejectedSamples == 5, "invalid luma never contaminates statistics");
}

void luminanceQuantiles() {
    auto data = dataset();
    for (auto& s : data) s.normalizedLuma.reset();
    data[0].normalizedLuma = 0;
    data[1].normalizedLuma = 0.2;
    data[2].normalizedLuma = 0.8;
    data[3].normalizedLuma = 1;
    const auto p = sessionFor(data).buildProfile();
    require(p.valid && p.luminance.sampleCount == 4, "only available luma counted");
    near(p.luminance.p10, 0.06); near(p.luminance.median, 0.5); near(p.luminance.p90, 0.94);
}

void impossibleFeatures() {
    HandCalibrationSession session;
    auto sample = observation(Pose::OPEN_HAND);
    sample.features.valid = false;
    require(session.addObservation(sample) == Result::INVALID_FEATURES, "valid flag required");
    for (auto field : {&HandFeatures::handScale, &HandFeatures::pinchRatio,
                       &HandFeatures::thumbSpreadRatio, &HandFeatures::thumbCurl,
                       &HandFeatures::indexCurl, &HandFeatures::middleCurl,
                       &HandFeatures::ringCurl, &HandFeatures::pinkyCurl}) {
        for (double bad : {-0.01, 1000.0}) {
            sample = observation(Pose::OPEN_HAND);
            sample.features.*field = bad;
            require(session.addObservation(sample) == Result::INVALID_FEATURES, "impossible range");
        }
    }
    sample = observation(Pose::OPEN_HAND);
    sample.features.handScale = 0;
    require(session.addObservation(sample) == Result::INVALID_FEATURES, "nonpositive scale");
    for (auto field : {&HandFeatures::thumbCurl, &HandFeatures::indexCurl,
                       &HandFeatures::middleCurl, &HandFeatures::ringCurl, &HandFeatures::pinkyCurl}) {
        sample = observation(Pose::OPEN_HAND);
        sample.features.*field = 1.001;
        require(session.addObservation(sample) == Result::INVALID_FEATURES, "curl above one");
    }
    sample = observation(Pose::OPEN_HAND);
    sample.features.pointerPoint.x = 17;
    require(session.addObservation(sample) == Result::INVALID_FEATURES, "corrupt coordinates");
}

void invalidConfig() {
    const auto check = [](HandCalibrationConfig config) {
        HandCalibrationSession session(config);
        require(!session.configValid() && !session.ready(), "invalid config not sanitized silently");
        require(session.addObservation(observation(Pose::OPEN_HAND)) == Result::INVALID_CONFIG,
                "invalid config rejects input");
        const auto p = session.buildProfile();
        require(p.status == Status::INVALID_CONFIG, "config failure status");
        noRecommendations(p);
    };
    auto config = HandCalibrationConfig{};
    config.minimumSamplesPerPose = 0; check(config);
    config = {}; config.maximumSamplesPerPose = 0; check(config);
    config = {}; config.maximumSamplesPerPose = 9; check(config);
    config = {}; config.maximumSamplesPerPose = std::numeric_limits<std::size_t>::max(); check(config);
    for (auto field : {&HandCalibrationConfig::minimumPinchMedianSeparation,
                       &HandCalibrationConfig::minimumPinchQuantileGap,
                       &HandCalibrationConfig::minimumCurlMedianSeparation,
                       &HandCalibrationConfig::minimumCurlQuantileGap}) {
        for (double bad : {nan, inf, -inf, 0.0, -1.0, 11.0}) {
            config = {}; config.*field = bad; check(config);
        }
    }
}

void configurableSeparation() {
    auto config = HandCalibrationConfig{};
    config.minimumPinchMedianSeparation = 0.9;
    require(sessionFor(dataset(), config).buildProfile().status == Status::PINCH_NOT_SEPARABLE,
            "pinch median separation configurable");
    config = {}; config.minimumPinchQuantileGap = 0.9;
    require(sessionFor(dataset(), config).buildProfile().status == Status::PINCH_NOT_SEPARABLE,
            "pinch quantile separation configurable");
    config = {}; config.minimumCurlMedianSeparation = 0.9;
    require(sessionFor(dataset(), config).buildProfile().status == Status::CURL_NOT_SEPARABLE,
            "curl median separation configurable");
    config = {}; config.minimumCurlQuantileGap = 0.9;
    require(sessionFor(dataset(), config).buildProfile().status == Status::CURL_NOT_SEPARABLE,
            "curl quantile separation configurable");
}

void capacityAndProgress() {
    HandCalibrationConfig config;
    config.minimumSamplesPerPose = config.maximumSamplesPerPose = 3;
    auto session = sessionFor(dataset(3), config);
    require(session.ready() && session.buildProfile().valid, "configured sample requirement");
    const auto before = session.buildProfile();
    for (auto pose : kHandCalibrationPoses) {
        require(session.addObservation(observation(pose)) == Result::POSE_CAPACITY_REACHED,
                "per-pose storage bounded");
        require(session.sampleCount(pose) == 3, "capacity does not overwrite accepted samples");
    }
    auto after = session.buildProfile();
    require(after.rejectedSamples == 4, "capacity rejection counted");
    after.rejectedSamples = 0;
    require(after == before, "capacity rejection does not change distributions");
}

void unsupportedPose() {
    HandCalibrationSession session;
    auto sample = observation(static_cast<Pose>(65000));
    require(session.addObservation(sample) == Result::UNSUPPORTED_POSE, "unknown pose rejected");
    require(session.sampleCount(sample.pose) == 0 && session.handedness() == Handedness::UNKNOWN,
            "invalid enum cannot index storage or lock hand");
}

void metadataDoesNotLockHand() {
    HandCalibrationSession session;
    auto sample = observation(Pose::OPEN_HAND);
    sample.normalizedLuma = nan;
    require(session.addObservation(sample) == Result::INVALID_LUMINANCE, "bad metadata rejected");
    require(session.handedness() == Handedness::UNKNOWN, "bad metadata cannot fix handedness");
    sample.normalizedLuma.reset(); sample.features.handedness = Handedness::LEFT;
    require(session.addObservation(sample) == Result::ACCEPTED &&
                session.handedness() == Handedness::LEFT, "first complete sample fixes hand");
}

void thumbAndConfidence() {
    auto data = dataset();
    for (auto& s : data) {
        s.features.thumbCurl = s.pose == Pose::OPEN_HAND ? 0.1 : 0.8;
        s.features.thumbSpreadRatio = s.pose == Pose::OPEN_HAND ? 1.5 : 0.5;
    }
    const auto p = sessionFor(data).buildProfile();
    require(p.valid && p.poses[0].thumbCurl.sampleCount == 10 &&
                p.poses[0].thumbSpreadRatio.sampleCount == 10, "thumb sample counts");
    near(p.poses[0].thumbCurl.median, 0.1); near(p.poses[0].thumbSpreadRatio.median, 1.5);
    near(p.poses[1].thumbCurl.median, 0.8); near(p.poses[1].thumbSpreadRatio.median, 0.5);
    require(p.trackingConfidence.sampleCount == 40, "confidence metadata counted");
    HandCalibrationSession session;
    for (double bad : {nan, inf, -inf, -0.1, 1.1}) {
        auto sample = observation(Pose::OPEN_HAND);
        sample.trackingConfidence = bad;
        require(session.addObservation(sample) == Result::INVALID_CONFIDENCE, "invalid confidence");
    }
}

void immutableBuildAndLifetime() {
    auto session = sessionFor(dataset());
    const auto before = session.buildProfile();
    require(session.buildProfile() == before && session.acceptedSamples() == 40,
            "buildProfile is repeatable and does not mutate session");
    require(session.addObservation(observation(Pose::OPEN_HAND)) == Result::ACCEPTED,
            "can continue acquisition after building");
    require(before.acceptedSamples == 40 && session.buildProfile().acceptedSamples == 41,
            "profile owns its summaries");
    const auto detached = sessionFor(dataset()).buildProfile();
    require(detached.valid && detached == before, "profile survives session destruction");
    static_assert(std::is_trivially_copyable_v<HandCalibrationProfile>);
    std::cout << "[INFO] sizeof(profile)=" << sizeof(HandCalibrationProfile)
              << ", sizeof(session)=" << sizeof(HandCalibrationSession)
              << ", sizeof(observation)=" << sizeof(HandCalibrationObservation) << '\n';
}

void ignoresPreclassifiedFlags() {
    auto data = dataset();
    const auto before = sessionFor(data).buildProfile();
    for (auto& s : data) {
        s.features.thumbExtended = s.features.indexExtended = s.features.middleExtended =
            s.features.ringExtended = s.features.pinkyExtended = true;
        s.features.palmCenter = {-0.1, 1.1, -2.0};
        s.features.pointerPoint = {1.2, -0.2, 3.0};
    }
    require(sessionFor(data).buildProfile() == before,
            "caller pose labels and raw curls, not runtime thresholds, define classes");
}

void emptySession() {
    HandCalibrationSession session;
    require(!session.ready() && session.configValid() && session.acceptedSamples() == 0 &&
                session.rejectedSamples() == 0, "initial session state");
    const auto p = session.buildProfile();
    noRecommendations(p);
    require(p.status == Status::INSUFFICIENT_SAMPLES && p.handScale.sampleCount == 0 &&
                p.handedness == Handedness::UNKNOWN, "empty profile diagnostics");
}

} // namespace

int main() {
    const std::pair<const char*, void (*)()> cases[]{
        {"valid profile", validProfile}, {"insufficient samples", insufficientSamples},
        {"NaN/Inf in every feature", nonFiniteFeatures}, {"handedness", handedness},
        {"type-7 percentiles", percentiles}, {"statistics edge cases", statisticEdgeCases},
        {"extreme accepted outliers", extremeOutlier}, {"pinch thresholds", pinchThresholds},
        {"pinch overlap", pinchOverlap}, {"pinch tail overlap", pinchTailOverlap},
        {"reversed pinch labels", reversedPinch}, {"curl thresholds", curlThresholds},
        {"curl overlap", curlOverlap}, {"curl tail overlap", curlTailOverlap},
        {"order independence", orderIndependence}, {"absent luminance", absentLuminance},
        {"invalid luminance", invalidLuminance}, {"luminance quantiles", luminanceQuantiles},
        {"impossible features", impossibleFeatures}, {"invalid config", invalidConfig},
        {"configurable separation", configurableSeparation}, {"capacity and progress", capacityAndProgress},
        {"unsupported pose", unsupportedPose}, {"metadata before hand lock", metadataDoesNotLockHand},
        {"thumb and confidence", thumbAndConfidence}, {"build and lifetime", immutableBuildAndLifetime},
        {"ignore preclassified flags", ignoresPreclassifiedFlags}, {"empty session", emptySession},
    };
    std::size_t failures = 0;
    for (const auto& [name, run] : cases) {
        try {
            run();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }
    std::cout << std::size(cases) - failures << '/' << std::size(cases) << " cases passed\n";
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
