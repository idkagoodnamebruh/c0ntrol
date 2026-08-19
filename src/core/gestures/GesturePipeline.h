#ifndef GESTUREPIPELINE_H
#define GESTUREPIPELINE_H

#include <array>
#include <cstddef>

#include "src/core/gestures/GestureEngine.h"
#include "src/core/gestures/GestureStateMachine.h"
#include "src/core/gestures/HandFeatureExtractor.h"

struct GesturePipelineResult {
    std::array<GestureObservation, 2> observations{};
    std::size_t observationCount{0};
    GestureEventBuffer<8> events{};
};

class GesturePipeline {
public:
    explicit GesturePipeline(GestureConfig config = {});

    GesturePipelineResult process(const HandTrackingFrame& frame);
    void reset();

private:
    GestureObservation observeHand(const TrackedHand* hand,
                                   Handedness expectedHandedness,
                                   const HandTrackingFrame& frame) const;

    HandFeatureExtractor m_featureExtractor;
    GestureEngine m_engine;
    GestureStateMachine m_leftStateMachine;
    GestureStateMachine m_rightStateMachine;
};

#endif // GESTUREPIPELINE_H
