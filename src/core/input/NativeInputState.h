#ifndef NATIVEINPUTSTATE_H
#define NATIVEINPUTSTATE_H

#include <cstddef>
#include <cstdint>
#include <string>

enum class NativeInputState {
    DISABLED,
    ACTIVATING,
    READY,
    FAILED,
    STOPPING,
};

struct NativeInputStatus {
    NativeInputState state{NativeInputState::DISABLED};
    bool desiredEnabled{false};
    std::uint64_t generation{0};
    std::string error;
    std::size_t pendingFrameCount{0};
    std::size_t pendingSemanticFrameCount{0};
    std::uint64_t droppedFrameCount{0};
    std::uint64_t droppedSemanticFrameCount{0};
};

inline const char* nativeInputStateName(NativeInputState state) {
    switch (state) {
        case NativeInputState::DISABLED: return "Disabled";
        case NativeInputState::ACTIVATING: return "Activating";
        case NativeInputState::READY: return "Ready";
        case NativeInputState::FAILED: return "Failed";
        case NativeInputState::STOPPING: return "Stopping";
    }
    return "Unknown";
}

#endif // NATIVEINPUTSTATE_H
