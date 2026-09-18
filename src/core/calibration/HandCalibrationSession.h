#ifndef HANDCALIBRATIONSESSION_H
#define HANDCALIBRATIONSESSION_H

#include <array>
#include <vector>

#include "HandCalibrationTypes.h"

// RESEARCH ONLY. No runtime consumer, I/O, image storage or landmark extraction.
class HandCalibrationSession {
public:
    explicit HandCalibrationSession(HandCalibrationConfig config = {});

    HandCalibrationSampleResult addObservation(const HandCalibrationObservation& observation);
    std::size_t acceptedSamples() const { return m_accepted; }
    std::size_t rejectedSamples() const { return m_rejected; }
    std::size_t sampleCount(HandCalibrationPose pose) const;
    Handedness handedness() const { return m_handedness; }
    bool configValid() const { return m_configValid; }
    // Acquisition readiness only; buildProfile() also checks class separation.
    bool ready() const;
    HandCalibrationProfile buildProfile() const;

private:
    struct Sample {
        double handScale;
        double pinchRatio;
        std::array<double, 4> fingerCurls;
        double thumbCurl;
        double thumbSpreadRatio;
        std::optional<double> normalizedLuma;
        std::optional<double> trackingConfidence;
    };

    HandCalibrationConfig m_config;
    bool m_configValid;
    Handedness m_handedness{Handedness::UNKNOWN};
    std::size_t m_accepted{0};
    std::size_t m_rejected{0};
    std::array<std::vector<Sample>, kHandCalibrationPoses.size()> m_samples;
};

#endif // HANDCALIBRATIONSESSION_H
