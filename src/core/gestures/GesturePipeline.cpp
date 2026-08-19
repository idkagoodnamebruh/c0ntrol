#include "GesturePipeline.h"

namespace {

void appendEvents(const GestureEventBatch& source,
                  GestureEventBuffer<8>& destination) {
    for (std::size_t i = 0; i < source.count; ++i)
        destination.push(source.events[i]);
}

} // namespace

GesturePipeline::GesturePipeline(GestureConfig config)
    : m_featureExtractor(config),
      m_engine(config),
      m_leftStateMachine(Handedness::LEFT, config),
      m_rightStateMachine(Handedness::RIGHT, config) {}

GestureObservation GesturePipeline::observeHand(
    const TrackedHand* hand, Handedness expectedHandedness,
    const HandTrackingFrame& frame) const {
    if (hand != nullptr) {
        HandFeatures features = m_featureExtractor.extract(*hand);
        GestureObservation observation =
            m_engine.observe(features, frame.frameId, frame.timestampUs);
        observation.handedness = expectedHandedness;
        return observation;
    }

    GestureObservation missing;
    missing.handedness = expectedHandedness;
    missing.frameId = frame.frameId;
    missing.timestampUs = frame.timestampUs;
    return missing;
}

GesturePipelineResult GesturePipeline::process(const HandTrackingFrame& frame) {
    const TrackedHand* leftHand = nullptr;
    const TrackedHand* rightHand = nullptr;
    std::size_t leftCount = 0;
    std::size_t rightCount = 0;

    if (frame.valid) {
        for (const auto& hand : frame.hands) {
            if (hand.handedness == Handedness::LEFT) {
                leftHand = &hand;
                ++leftCount;
            } else if (hand.handedness == Handedness::RIGHT) {
                rightHand = &hand;
                ++rightCount;
            }
        }
    }

    // A duplicate or UNKNOWN handedness is ambiguous. Treat that nominal slot
    // as missing instead of allowing one hand to mutate the other hand's FSM.
    const GestureObservation left = observeHand(
        leftCount == 1 ? leftHand : nullptr, Handedness::LEFT, frame);
    const GestureObservation right = observeHand(
        rightCount == 1 ? rightHand : nullptr, Handedness::RIGHT, frame);

    GesturePipelineResult result;
    if (left.valid) result.observations[result.observationCount++] = left;
    if (right.valid) result.observations[result.observationCount++] = right;

    appendEvents(m_leftStateMachine.update(left), result.events);
    appendEvents(m_rightStateMachine.update(right), result.events);
    return result;
}

void GesturePipeline::reset() {
    m_leftStateMachine.reset();
    m_rightStateMachine.reset();
}
