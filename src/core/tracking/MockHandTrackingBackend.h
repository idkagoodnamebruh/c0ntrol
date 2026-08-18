#ifndef MOCKHANDTRACKINGBACKEND_H
#define MOCKHANDTRACKINGBACKEND_H

#include <cmath>

#include "src/core/tracking/IHandTrackingBackend.h"

class MockHandTrackingBackend final : public IHandTrackingBackend {
public:
    bool initialize(const HandTrackingConfig&) override {
        m_initialized = true;
        m_error.clear();
        return true;
    }

    HandTrackingFrame process(const RgbImageView&, std::int64_t timestampUs,
                              std::uint64_t frameId) override {
        HandTrackingFrame frame;
        frame.timestampUs = timestampUs;
        frame.frameId = frameId;
        if (!m_initialized) {
            m_error = "Mock backend is not initialized";
            return frame;
        }

        const double t = static_cast<double>(timestampUs) / 1'000'000.0;
        TrackedHand hand;
        hand.handedness = Handedness::RIGHT;
        hand.handednessScore = 1.0F;
        const double baseX = 0.5 + 0.2 * std::sin(t);
        const double baseY = 0.5 + 0.2 * std::cos(t);
        for (std::size_t i = 0; i < hand.landmarks.size(); ++i) {
            hand.landmarks[i] = Point3D(baseX + i * 0.01 * std::sin(t * 2.0),
                                        baseY + i * 0.01 * std::cos(t * 2.0), 0.0);
        }
        frame.hands.push_back(hand);
        frame.valid = true;
        return frame;
    }

    void shutdown() override { m_initialized = false; }
    std::string lastError() const override { return m_error; }

private:
    bool m_initialized{false};
    std::string m_error;
};

#endif // MOCKHANDTRACKINGBACKEND_H
