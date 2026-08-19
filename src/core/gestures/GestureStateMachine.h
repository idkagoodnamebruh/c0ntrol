#ifndef GESTURESTATEMACHINE_H
#define GESTURESTATEMACHINE_H

#include "src/core/gestures/GestureTypes.h"

enum class PinchState {
    IDLE,
    CANDIDATE_DOWN,
    PINCHED,
    CANDIDATE_UP,
};

class GestureStateMachine {
public:
    explicit GestureStateMachine(Handedness handedness,
                                 GestureConfig config = {});

    GestureEventBatch update(const GestureObservation& observation);
    void reset();

    PinchState pinchState() const { return m_pinchState; }
    bool pointerActive() const { return m_pointerActive; }

private:
    GestureEvent event(GestureEventType type, std::uint64_t frameId,
                       std::int64_t timestampUs, const Point3D& pointer) const;
    GestureEventBatch processMissing(const GestureObservation& observation);
    void processPointer(const GestureObservation& observation,
                        GestureEventBatch& output);
    void processPinch(const GestureObservation& observation,
                      GestureEventBatch& output);

    Handedness m_handedness;
    GestureConfig m_config;
    PinchState m_pinchState{PinchState::IDLE};
    std::int64_t m_candidateSinceUs{0};
    std::int64_t m_trackingLostSinceUs{0};
    std::int64_t m_lastTimestampUs{0};
    Point3D m_lastPointer{};
    bool m_pointerActive{false};
    bool m_trackingLost{false};
    bool m_hasTimestamp{false};
};

#endif // GESTURESTATEMACHINE_H
