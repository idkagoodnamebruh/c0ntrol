#ifndef GESTUREPIPELINE_H
#define GESTUREPIPELINE_H

#include <array>
#include <cstddef>

#include "src/core/gestures/DynamicGestureRecognizer.h"
#include "src/core/gestures/GestureEngine.h"
#include "src/core/gestures/GestureStateMachine.h"
#include "src/core/gestures/HandFeatureExtractor.h"

struct GesturePipelineResult {
    std::array<GestureObservation, 2> observations{};
    std::size_t observationCount{0};
    GestureEventBuffer<12> events{};
};

class GesturePipeline {
public:
    explicit GesturePipeline(GestureConfig config = {},
                             DynamicGestureConfig dynamicConfig = {});

    GesturePipelineResult process(const HandTrackingFrame& frame);
    void reset();

private:
    struct HandResult {
        HandFeatures features{};
        GestureObservation observation{};
    };

    HandResult observeHand(const TrackedHand* hand,
                           Handedness expectedHandedness,
                           const HandTrackingFrame& frame) const;
    void processHand(const HandResult& hand,
                     GestureStateMachine& stateMachine,
                     DynamicGestureRecognizer& recognizer,
                     GesturePipelineResult& result);

    HandFeatureExtractor m_featureExtractor;
    GestureEngine m_engine;
    GestureStateMachine m_leftStateMachine;
    GestureStateMachine m_rightStateMachine;
    DynamicGestureRecognizer m_leftDynamicRecognizer;
    DynamicGestureRecognizer m_rightDynamicRecognizer;
};

#endif // GESTUREPIPELINE_H
