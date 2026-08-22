#ifndef ACTIONTYPES_H
#define ACTIONTYPES_H

#include <cstdint>

#include "src/core/tracking/HandTrackingTypes.h"

enum class ActionType {
    MOVE_POINTER,
    PRIMARY_BUTTON_DOWN,
    PRIMARY_BUTTON_UP,
};

struct DesktopPoint {
    int x{0};
    int y{0};
};

struct DesktopGeometry {
    int originX{0};
    int originY{0};
    int width{0};
    int height{0};

    bool isValid() const { return width > 0 && height > 0; }
};

struct ActionCommand {
    ActionType type{ActionType::MOVE_POINTER};
    DesktopPoint desktopPoint{};
    Handedness handedness{Handedness::UNKNOWN};
    std::uint64_t frameId{0};
    std::int64_t timestampUs{0};
};

#endif // ACTIONTYPES_H
