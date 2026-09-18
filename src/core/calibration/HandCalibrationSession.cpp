#include "HandCalibrationSession.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include "HandCalibrationStatistics.h"

namespace {

std::optional<std::size_t> poseIndex(HandCalibrationPose pose) {
    for (std::size_t i = 0; i < kHandCalibrationPoses.size(); ++i) {
        if (kHandCalibrationPoses[i] == pose) return i;
    }
    return std::nullopt;
}

bool inRange(double value, double low, double high) {
    return std::isfinite(value) && value >= low && value <= high;
}

bool validConfig(const HandCalibrationConfig& config) {
    // Bound allocation and arithmetic even for untrusted size_t configuration.
    return config.minimumSamplesPerPose > 0 &&
           config.minimumSamplesPerPose <= config.maximumSamplesPerPose &&
           config.maximumSamplesPerPose <= 4096 &&
           inRange(config.minimumPinchMedianSeparation, 1e-6, 10.0) &&
           inRange(config.minimumPinchQuantileGap, 1e-6, 10.0) &&
           inRange(config.minimumCurlMedianSeparation, 1e-6, 1.0) &&
           inRange(config.minimumCurlQuantileGap, 1e-6, 1.0);
}

bool validPoint(const Point3D& point) {
    // Image-normalized coordinates may extend outside the frame, especially z.
    // This is a generous corruption guard, not an in-frame visibility test.
    return inRange(point.x, -16.0, 16.0) &&
           inRange(point.y, -16.0, 16.0) &&
           inRange(point.z, -16.0, 16.0);
}

bool validFeatures(const HandFeatures& f) {
    return f.valid && inRange(f.handScale, 0.0, 4.0) && f.handScale > 0.0 &&
           validPoint(f.palmCenter) && validPoint(f.pointerPoint) &&
           inRange(f.pinchRatio, 0.0, 10.0) &&
           inRange(f.thumbSpreadRatio, 0.0, 10.0) &&
           inRange(f.thumbCurl, 0.0, 1.0) && inRange(f.indexCurl, 0.0, 1.0) &&
           inRange(f.middleCurl, 0.0, 1.0) && inRange(f.ringCurl, 0.0, 1.0) &&
           inRange(f.pinkyCurl, 0.0, 1.0);
}

std::optional<std::pair<double, double>> thresholds(
    const HandCalibrationStatistics& low, const HandCalibrationStatistics& high,
    double minimumMedianSeparation, double minimumQuantileGap) {
    if (low.sampleCount == 0 || high.sampleCount == 0 ||
        high.median - low.median < minimumMedianSeparation ||
        high.p10 - low.p90 < minimumQuantileGap) {
        return std::nullopt;
    }
    const double lower = std::lerp(low.p90, high.p10, 1.0 / 3.0);
    const double upper = std::lerp(low.p90, high.p10, 2.0 / 3.0);
    if (!(low.p90 < lower && lower < upper && upper < high.p10))
        return std::nullopt;
    return std::pair{lower, upper};
}

} // namespace

HandCalibrationSession::HandCalibrationSession(HandCalibrationConfig config)
    : m_config(config), m_configValid(validConfig(config)) {}

HandCalibrationSampleResult HandCalibrationSession::addObservation(
    const HandCalibrationObservation& observation) {
    const auto reject = [this](HandCalibrationSampleResult reason) {
        // Saturation is defined even if a caller sends an unbounded invalid stream.
        if (m_rejected < std::numeric_limits<std::size_t>::max()) ++m_rejected;
        return reason;
    };
    if (!m_configValid) return reject(HandCalibrationSampleResult::INVALID_CONFIG);
    const auto index = poseIndex(observation.pose);
    if (!index) return reject(HandCalibrationSampleResult::UNSUPPORTED_POSE);
    const auto& f = observation.features;
    if (!validFeatures(f)) return reject(HandCalibrationSampleResult::INVALID_FEATURES);
    if (f.handedness != Handedness::LEFT && f.handedness != Handedness::RIGHT)
        return reject(HandCalibrationSampleResult::INVALID_HANDEDNESS);
    if (m_handedness != Handedness::UNKNOWN && m_handedness != f.handedness)
        return reject(HandCalibrationSampleResult::OPPOSITE_HAND);
    if (observation.normalizedLuma && !inRange(*observation.normalizedLuma, 0.0, 1.0))
        return reject(HandCalibrationSampleResult::INVALID_LUMINANCE);
    if (observation.trackingConfidence &&
        !inRange(*observation.trackingConfidence, 0.0, 1.0))
        return reject(HandCalibrationSampleResult::INVALID_CONFIDENCE);
    auto& samples = m_samples[*index];
    if (samples.size() >= m_config.maximumSamplesPerPose)
        return reject(HandCalibrationSampleResult::POSE_CAPACITY_REACHED);
    if (samples.size() == samples.capacity()) {
        // Geometric growth, bounded by the configured cap; no allocation per sample.
        const auto next = samples.empty() ? m_config.minimumSamplesPerPose
                                         : samples.size() * 2;
        samples.reserve(std::min(next, m_config.maximumSamplesPerPose));
    }
    samples.push_back({f.handScale, f.pinchRatio,
                       {f.indexCurl, f.middleCurl, f.ringCurl, f.pinkyCurl},
                       f.thumbCurl, f.thumbSpreadRatio,
                       observation.normalizedLuma, observation.trackingConfidence});
    // Only a fully validated and stored sample fixes the session's handedness.
    m_handedness = f.handedness;
    ++m_accepted;
    return HandCalibrationSampleResult::ACCEPTED;
}

std::size_t HandCalibrationSession::sampleCount(HandCalibrationPose pose) const {
    const auto index = poseIndex(pose);
    return index ? m_samples[*index].size() : 0;
}

bool HandCalibrationSession::ready() const {
    if (!m_configValid) return false;
    for (const auto& samples : m_samples) {
        if (samples.size() < m_config.minimumSamplesPerPose) return false;
    }
    return true;
}

HandCalibrationProfile HandCalibrationSession::buildProfile() const {
    HandCalibrationProfile profile;
    profile.acceptedSamples = m_accepted;
    profile.rejectedSamples = m_rejected;
    profile.handedness = m_handedness;
    if (!m_configValid) {
        profile.status = HandCalibrationProfileStatus::INVALID_CONFIG;
        return profile;
    }

    // Reuse one scratch buffer for every distribution; retained session data is immutable.
    std::vector<double> scratch;
    scratch.reserve(std::max(m_accepted,
                            4 * sampleCount(HandCalibrationPose::OPEN_HAND) +
                                sampleCount(HandCalibrationPose::POINTING)));
    const auto collect = [&](auto append) {
        scratch.clear();
        for (std::size_t i = 0; i < m_samples.size(); ++i) {
            for (const auto& sample : m_samples[i]) {
                append(kHandCalibrationPoses[i], sample, scratch);
            }
        }
        // Samples are private, bounded and validated at insertion.
        return hand_calibration::summarize(scratch).value();
    };
    profile.handScale = collect([](auto, const Sample& s, auto& out) {
        out.push_back(s.handScale);
    });
    profile.luminance = collect([](auto, const Sample& s, auto& out) {
        if (s.normalizedLuma) out.push_back(*s.normalizedLuma);
    });
    profile.trackingConfidence = collect([](auto, const Sample& s, auto& out) {
        if (s.trackingConfidence) out.push_back(*s.trackingConfidence);
    });
    for (std::size_t i = 0; i < m_samples.size(); ++i) {
        auto& pose = profile.poses[i];
        pose.sampleCount = m_samples[i].size();
        pose.thumbCurl = collect([i](auto label, const Sample& s, auto& out) {
            if (label == kHandCalibrationPoses[i]) out.push_back(s.thumbCurl);
        });
        pose.thumbSpreadRatio = collect([i](auto label, const Sample& s, auto& out) {
            if (label == kHandCalibrationPoses[i]) out.push_back(s.thumbSpreadRatio);
        });
    }
    profile.pinchClosed = collect([](auto pose, const Sample& s, auto& out) {
        if (pose == HandCalibrationPose::PINCH_CLOSED) out.push_back(s.pinchRatio);
    });
    profile.pinchOpen = collect([](auto pose, const Sample& s, auto& out) {
        if (pose == HandCalibrationPose::PINCH_OPEN) out.push_back(s.pinchRatio);
    });
    profile.extendedCurl = collect([](auto pose, const Sample& s, auto& out) {
        // Four extended fingers in OPEN_HAND, only index in POINTING.
        if (pose == HandCalibrationPose::OPEN_HAND)
            out.insert(out.end(), s.fingerCurls.begin(), s.fingerCurls.end());
        else if (pose == HandCalibrationPose::POINTING)
            out.push_back(s.fingerCurls[0]);
    });
    profile.curledCurl = collect([](auto pose, const Sample& s, auto& out) {
        // POINTING index is extended: never use it as a flexed example.
        if (pose == HandCalibrationPose::POINTING)
            out.insert(out.end(), s.fingerCurls.begin() + 1, s.fingerCurls.end());
    });
    if (!ready()) return profile;
    const auto pinch = thresholds(profile.pinchClosed, profile.pinchOpen,
                                  m_config.minimumPinchMedianSeparation,
                                  m_config.minimumPinchQuantileGap);
    if (!pinch) {
        profile.status = HandCalibrationProfileStatus::PINCH_NOT_SEPARABLE;
        return profile;
    }
    const auto curl = thresholds(profile.extendedCurl, profile.curledCurl,
                                 m_config.minimumCurlMedianSeparation,
                                 m_config.minimumCurlQuantileGap);
    if (!curl) {
        profile.status = HandCalibrationProfileStatus::CURL_NOT_SEPARABLE;
        return profile;
    }
    profile.recommendedPinchEnterRatio = pinch->first;
    profile.recommendedPinchExitRatio = pinch->second;
    profile.recommendedFingerExtendedMaxCurl = curl->first;
    profile.recommendedFingerCurledMinCurl = curl->second;
    profile.valid = true;
    profile.status = HandCalibrationProfileStatus::VALID;
    return profile;
}
