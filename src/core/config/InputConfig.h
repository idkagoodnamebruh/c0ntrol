#ifndef INPUTCONFIG_H
#define INPUTCONFIG_H

#include "src/core/tracking/HandTrackingTypes.h"

struct InputConfig {
    // A first run must never inject pointer/button input without consent.
    bool enabled{false};
    Handedness preferredHand{Handedness::RIGHT};

    bool operator==(const InputConfig&) const = default;
};

inline InputConfig sanitizeInputConfig(InputConfig config) {
    if (config.preferredHand != Handedness::LEFT &&
        config.preferredHand != Handedness::RIGHT) {
        config.preferredHand = InputConfig{}.preferredHand;
    }
    return config;
}

#endif // INPUTCONFIG_H
