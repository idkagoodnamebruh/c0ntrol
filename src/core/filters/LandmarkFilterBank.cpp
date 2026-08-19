#include "LandmarkFilterBank.h"

#include <cmath>

LandmarkFilterBank::LandmarkChannels::LandmarkChannels(
    const OneEuroConfig& config) {
    for (auto& landmark : filters) {
        for (auto& axis : landmark) axis.configure(config);
    }
}

Point3D LandmarkFilterBank::LandmarkChannels::filterPoint(
    const Point3D& point, std::size_t landmarkIndex, double timestampSeconds) {
    auto& axes = filters[landmarkIndex];
    return {
        axes[0].filter(point.x, timestampSeconds),
        axes[1].filter(point.y, timestampSeconds),
        axes[2].filter(point.z, timestampSeconds),
    };
}

void LandmarkFilterBank::LandmarkChannels::reset() {
    for (auto& landmark : filters) {
        for (auto& axis : landmark) axis.reset();
    }
}

LandmarkFilterBank::HandState::HandState(
    const OneEuroConfig& normalizedConfig, const OneEuroConfig& worldConfig)
    : normalized(normalizedConfig), world(worldConfig) {}

void LandmarkFilterBank::HandState::reset() {
    normalized.reset();
    world.reset();
    lastSeenTimestampUs = 0;
    lastWrist = {};
    active = false;
    hasFiniteWrist = false;
}

LandmarkFilterConfig LandmarkFilterBank::sanitizeConfig(
    LandmarkFilterConfig config) {
    if (config.handResetTimeoutUs <= 0) config.handResetTimeoutUs = 400'000;
    if (!std::isfinite(config.teleportThreshold) ||
        config.teleportThreshold <= 0.0) {
        config.teleportThreshold = 0.35;
    }
    return config;
}

LandmarkFilterBank::LandmarkFilterBank(LandmarkFilterConfig config)
    : m_config(sanitizeConfig(config)),
      m_left(m_config.normalized, m_config.world),
      m_right(m_config.normalized, m_config.world) {}

bool LandmarkFilterBank::isFinite(const Point3D& point) {
    return std::isfinite(point.x) && std::isfinite(point.y) &&
           std::isfinite(point.z);
}

LandmarkFilterBank::HandState& LandmarkFilterBank::stateFor(
    Handedness handedness) {
    return handedness == Handedness::LEFT ? m_left : m_right;
}

void LandmarkFilterBank::expireStaleHands(std::int64_t timestampUs) {
    if (timestampUs < 0) return;
    for (auto* state : {&m_left, &m_right}) {
        if (!state->active || timestampUs <= state->lastSeenTimestampUs) continue;
        if (timestampUs - state->lastSeenTimestampUs >=
            m_config.handResetTimeoutUs) {
            state->reset();
        }
    }
}

void LandmarkFilterBank::filterHand(
    TrackedHand& outputHand, const TrackedHand& rawHand, HandState& state,
    std::int64_t timestampUs, double timestampSeconds) {
    if (state.active && timestampUs <= state.lastSeenTimestampUs) {
        state.reset();
    }

    const Point3D& wrist = rawHand.landmarks[0];
    const bool finiteWrist = isFinite(wrist);
    if (state.active && state.hasFiniteWrist && finiteWrist &&
        wrist.distanceTo(state.lastWrist) > m_config.teleportThreshold) {
        state.reset();
    }

    for (std::size_t i = 0; i < rawHand.landmarks.size(); ++i) {
        outputHand.landmarks[i] =
            state.normalized.filterPoint(rawHand.landmarks[i], i, timestampSeconds);
    }

    if (rawHand.worldLandmarks) {
        auto& outputWorld = *outputHand.worldLandmarks;
        for (std::size_t i = 0; i < rawHand.worldLandmarks->size(); ++i) {
            outputWorld[i] = state.world.filterPoint(
                (*rawHand.worldLandmarks)[i], i, timestampSeconds);
        }
    }

    state.lastSeenTimestampUs = timestampUs;
    state.active = true;
    if (finiteWrist) {
        state.lastWrist = wrist;
        state.hasFiniteWrist = true;
    }
}

HandTrackingFrame LandmarkFilterBank::process(
    const HandTrackingFrame& rawFrame) {
    HandTrackingFrame filteredFrame = rawFrame;

    if (!rawFrame.valid) {
        filteredFrame.hands.clear();
        expireStaleHands(rawFrame.timestampUs);
        return filteredFrame;
    }

    if (!m_config.enabled) return filteredFrame;

    if (rawFrame.timestampUs < 0) {
        reset();
        return filteredFrame;
    }

    expireStaleHands(rawFrame.timestampUs);

    std::size_t leftCount = 0;
    std::size_t rightCount = 0;
    bool hasUnknown = false;
    for (const auto& hand : rawFrame.hands) {
        if (hand.handedness == Handedness::LEFT) ++leftCount;
        else if (hand.handedness == Handedness::RIGHT) ++rightCount;
        else hasUnknown = true;
    }

    if (hasUnknown) reset();
    if (leftCount > 1) resetHand(Handedness::LEFT);
    if (rightCount > 1) resetHand(Handedness::RIGHT);

    const double timestampSeconds =
        static_cast<double>(rawFrame.timestampUs) / 1'000'000.0;
    for (std::size_t i = 0; i < rawFrame.hands.size(); ++i) {
        const auto handedness = rawFrame.hands[i].handedness;
        const bool uniqueLeft = handedness == Handedness::LEFT && leftCount == 1;
        const bool uniqueRight = handedness == Handedness::RIGHT && rightCount == 1;
        if (!uniqueLeft && !uniqueRight) continue;

        filterHand(filteredFrame.hands[i], rawFrame.hands[i],
                   stateFor(handedness), rawFrame.timestampUs,
                   timestampSeconds);
    }
    return filteredFrame;
}

void LandmarkFilterBank::reset() {
    m_left.reset();
    m_right.reset();
}

void LandmarkFilterBank::resetHand(Handedness handedness) {
    if (handedness == Handedness::LEFT) m_left.reset();
    else if (handedness == Handedness::RIGHT) m_right.reset();
    else reset();
}

void LandmarkFilterBank::setEnabled(bool enabled) {
    if (m_config.enabled == enabled) return;
    m_config.enabled = enabled;
    reset();
}
