#include "GestureStateMachine.h"

#include <cmath>

GestureStateMachine::GestureStateMachine(Handedness handedness,
                                         GestureConfig config)
    : m_handedness(handedness),
      m_config(sanitizeGestureConfig(config)) {}

GestureEvent GestureStateMachine::event(
    GestureEventType type, std::uint64_t frameId, std::int64_t timestampUs,
    const Point3D& pointer) const {
    return {type, m_handedness, frameId, timestampUs, pointer};
}

void GestureStateMachine::processPointer(
    const GestureObservation& observation, GestureEventBatch& output) {
    m_lastPointer = observation.pointerPoint;
    if (observation.pointerActive && !m_pointerActive) {
        output.push(event(GestureEventType::POINTER_ACTIVE,
                          observation.frameId, observation.timestampUs,
                          observation.pointerPoint));
    } else if (!observation.pointerActive && m_pointerActive) {
        output.push(event(GestureEventType::POINTER_INACTIVE,
                          observation.frameId, observation.timestampUs,
                          observation.pointerPoint));
    }
    m_pointerActive = observation.pointerActive;
}

void GestureStateMachine::processPinch(
    const GestureObservation& observation, GestureEventBatch& output) {
    const bool belowEnter =
        std::isfinite(observation.pinchRatio) &&
        observation.pinchRatio <= m_config.pinchEnterRatio;
    const bool aboveExit =
        !std::isfinite(observation.pinchRatio) ||
        observation.pinchRatio >= m_config.pinchExitRatio;

    switch (m_pinchState) {
        case PinchState::IDLE:
            if (belowEnter) {
                m_pinchState = PinchState::CANDIDATE_DOWN;
                m_candidateSinceUs = observation.timestampUs;
            }
            break;
        case PinchState::CANDIDATE_DOWN:
            if (!belowEnter) {
                m_pinchState = PinchState::IDLE;
            } else if (observation.timestampUs - m_candidateSinceUs >=
                       m_config.pinchEnterHoldUs) {
                m_pinchState = PinchState::PINCHED;
                output.push(event(GestureEventType::PINCH_BEGIN,
                                  observation.frameId,
                                  observation.timestampUs,
                                  observation.pointerPoint));
            }
            break;
        case PinchState::PINCHED:
            if (aboveExit) {
                m_pinchState = PinchState::CANDIDATE_UP;
                m_candidateSinceUs = observation.timestampUs;
            }
            break;
        case PinchState::CANDIDATE_UP:
            if (!aboveExit) {
                m_pinchState = PinchState::PINCHED;
            } else if (observation.timestampUs - m_candidateSinceUs >=
                       m_config.pinchExitHoldUs) {
                m_pinchState = PinchState::IDLE;
                output.push(event(GestureEventType::PINCH_END,
                                  observation.frameId,
                                  observation.timestampUs,
                                  observation.pointerPoint));
            }
            break;
    }
}

GestureEventBatch GestureStateMachine::processMissing(
    const GestureObservation& observation) {
    GestureEventBatch output;
    if (m_pointerActive) {
        output.push(event(GestureEventType::POINTER_INACTIVE,
                          observation.frameId, observation.timestampUs,
                          m_lastPointer));
        m_pointerActive = false;
    }

    if (!m_trackingLost) {
        m_trackingLost = true;
        m_trackingLostSinceUs = observation.timestampUs;
        if (m_pinchState == PinchState::CANDIDATE_DOWN)
            m_pinchState = PinchState::IDLE;
        else if (m_pinchState == PinchState::CANDIDATE_UP)
            m_pinchState = PinchState::PINCHED;
    }

    if (observation.timestampUs - m_trackingLostSinceUs >=
        m_config.trackingLostTimeoutUs) {
        if (m_pinchState == PinchState::PINCHED) {
            output.push(event(GestureEventType::PINCH_CANCEL,
                              observation.frameId, observation.timestampUs,
                              m_lastPointer));
        }
        m_pinchState = PinchState::IDLE;
    }
    return output;
}

GestureEventBatch GestureStateMachine::update(
    const GestureObservation& observation) {
    GestureEventBatch output;
    if (observation.timestampUs < 0 ||
        (m_hasTimestamp && observation.timestampUs <= m_lastTimestampUs)) {
        return output;
    }
    m_lastTimestampUs = observation.timestampUs;
    m_hasTimestamp = true;

    if (!observation.valid || observation.handedness != m_handedness) {
        return processMissing(observation);
    }

    m_trackingLost = false;
    processPointer(observation, output);
    processPinch(observation, output);
    return output;
}

void GestureStateMachine::reset() {
    m_pinchState = PinchState::IDLE;
    m_candidateSinceUs = 0;
    m_trackingLostSinceUs = 0;
    m_lastTimestampUs = 0;
    m_lastPointer = {};
    m_pointerActive = false;
    m_trackingLost = false;
    m_hasTimestamp = false;
}
