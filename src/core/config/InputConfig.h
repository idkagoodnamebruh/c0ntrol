#ifndef INPUTCONFIG_H
#define INPUTCONFIG_H

#include <algorithm>

#include "src/core/tracking/HandTrackingTypes.h"

struct InputConfig {
    // A first run must never inject pointer/button input without consent.
    bool enabled{false};
    Handedness preferredHand{Handedness::RIGHT};
    bool swipeScrollEnabled{true};
    int scrollNotchesPerSwipe{3};
    bool invertSwipeScroll{false};

    bool operator==(const InputConfig&) const = default;
};

inline InputConfig sanitizeInputConfig(InputConfig config) {
    if (config.preferredHand != Handedness::LEFT &&
        config.preferredHand != Handedness::RIGHT) {
        config.preferredHand = InputConfig{}.preferredHand;
    }
    if (config.scrollNotchesPerSwipe < 1 ||
        config.scrollNotchesPerSwipe > 10) {
        config.scrollNotchesPerSwipe = InputConfig{}.scrollNotchesPerSwipe;
    }
    return config;
}

#endif // INPUTCONFIG_H
