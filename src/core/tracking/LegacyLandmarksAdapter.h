#ifndef LEGACYLANDMARKSADAPTER_H
#define LEGACYLANDMARKSADAPTER_H

#include "src/core/tracking/HandTrackingTypes.h"

inline const TrackedHand* selectLegacyHand(const HandTrackingFrame& frame) {
    if (!frame.valid || frame.hands.empty()) return nullptr;
    for (const auto& hand : frame.hands) {
        if (hand.handedness == Handedness::RIGHT) return &hand;
    }
    return &frame.hands.front();
}

inline Landmarks toLegacyLandmarks(const HandTrackingFrame& frame) {
    Landmarks result;
    const auto* hand = selectLegacyHand(frame);
    if (!hand) return result;
    result.points.assign(hand->landmarks.begin(), hand->landmarks.end());
    return result;
}

#endif // LEGACYLANDMARKSADAPTER_H
