#ifndef LANDMARKFILTERBANK_H
#define LANDMARKFILTERBANK_H

#include <array>
#include <cstddef>
#include <cstdint>

#include "src/core/filters/OneEuroFilter.h"
#include "src/core/tracking/HandTrackingTypes.h"

struct LandmarkFilterConfig {
    OneEuroConfig normalized{};
    OneEuroConfig world{};
    std::int64_t handResetTimeoutUs{400'000};
    double teleportThreshold{0.35};
    bool enabled{true};
};

class LandmarkFilterBank {
public:
    explicit LandmarkFilterBank(LandmarkFilterConfig config = {});

    HandTrackingFrame process(const HandTrackingFrame& rawFrame);

    void reset();
    void resetHand(Handedness handedness);
    void setEnabled(bool enabled);

    bool enabled() const { return m_config.enabled; }
    const LandmarkFilterConfig& config() const { return m_config; }

private:
    struct LandmarkChannels {
        explicit LandmarkChannels(const OneEuroConfig& config);

        Point3D filterPoint(const Point3D& point, std::size_t landmarkIndex,
                            double timestampSeconds);
        void reset();

        std::array<std::array<OneEuroFilter, 3>, 21> filters{};
    };

    struct HandState {
        HandState(const OneEuroConfig& normalizedConfig,
                  const OneEuroConfig& worldConfig);

        void reset();

        LandmarkChannels normalized;
        LandmarkChannels world;
        std::int64_t lastSeenTimestampUs{0};
        Point3D lastWrist{};
        bool active{false};
        bool hasFiniteWrist{false};
    };

    static LandmarkFilterConfig sanitizeConfig(LandmarkFilterConfig config);
    static bool isFinite(const Point3D& point);

    HandState& stateFor(Handedness handedness);
    void expireStaleHands(std::int64_t timestampUs);
    void filterHand(TrackedHand& outputHand, const TrackedHand& rawHand,
                    HandState& state, std::int64_t timestampUs,
                    double timestampSeconds);

    LandmarkFilterConfig m_config;
    HandState m_left;
    HandState m_right;
};

#endif // LANDMARKFILTERBANK_H
