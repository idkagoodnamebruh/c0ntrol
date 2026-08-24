#include "GesturePipeline.h"

namespace {

void appendEvents(const GestureEventBatch& source,
                  GestureEventBuffer<12>& destination) {
    for (std::size_t i = 0; i < source.count; ++i)
        destination.push(source.events[i]);
}

} // namespace

GesturePipeline::GesturePipeline(GestureConfig config,
                                 DynamicGestureConfig dynamicConfig)
    : m_featureExtractor(config),
      m_engine(config),
      m_leftStateMachine(Handedness::LEFT, config),
      m_rightStateMachine(Handedness::RIGHT, config),
      m_leftDynamicRecognizer(Handedness::LEFT, dynamicConfig),
      m_rightDynamicRecognizer(Handedness::RIGHT, dynamicConfig) {}

GesturePipeline::HandResult GesturePipeline::observeHand(
    const TrackedHand* hand, Handedness expectedHandedness,
    const HandTrackingFrame& frame) const {
    if (hand != nullptr) {
        HandResult result;
        result.features = m_featureExtractor.extract(*hand);
        result.observation = m_engine.observe(
            result.features, frame.frameId, frame.timestampUs);
        result.observation.handedness = expectedHandedness;
        return result;
    }

    HandResult result;
    result.observation.handedness = expectedHandedness;
    result.observation.frameId = frame.frameId;
    result.observation.timestampUs = frame.timestampUs;
    return result;
}

void GesturePipeline::processHand(
    const HandResult& hand, GestureStateMachine& stateMachine,
    DynamicGestureRecognizer& recognizer, GesturePipelineResult& result) {
    if (hand.observation.valid)
        result.observations[result.observationCount++] = hand.observation;
    appendEvents(stateMachine.update(hand.observation), result.events);
    if (!hand.observation.valid) {
        recognizer.reset();
        return;
    }
    const auto dynamicEvent = recognizer.update(
        hand.features, hand.observation.pose,
        hand.observation.frameId, hand.observation.timestampUs);
    if (dynamicEvent.has_value()) result.events.push(*dynamicEvent);
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
    const HandResult left = observeHand(
        leftCount == 1 ? leftHand : nullptr, Handedness::LEFT, frame);
    const HandResult right = observeHand(
        rightCount == 1 ? rightHand : nullptr, Handedness::RIGHT, frame);

    GesturePipelineResult result;
    processHand(left, m_leftStateMachine, m_leftDynamicRecognizer, result);
    processHand(right, m_rightStateMachine, m_rightDynamicRecognizer, result);
    return result;
}

void GesturePipeline::reset() {
    m_leftStateMachine.reset();
    m_rightStateMachine.reset();
    m_leftDynamicRecognizer.reset();
    m_rightDynamicRecognizer.reset();
}
