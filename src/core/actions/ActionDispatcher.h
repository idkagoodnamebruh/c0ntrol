#ifndef ACTIONDISPATCHER_H
#define ACTIONDISPATCHER_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "src/core/actions/ISystemInputBackend.h"
#include "src/core/actions/PointerMapper.h"
#include "src/core/config/InputConfig.h"
#include "src/core/gestures/GesturePipeline.h"

struct ActionDispatchResult {
    bool success{true};
    std::array<ActionCommand, 8> commands{};
    std::size_t commandCount{0};
    std::string error;

    void record(const ActionCommand& command) {
        if (commandCount < commands.size()) commands[commandCount++] = command;
    }
};

class ActionDispatcher {
public:
    explicit ActionDispatcher(ISystemInputBackend& backend,
                              PointerMappingConfig mappingConfig = {},
                              InputConfig inputConfig = {});
    ~ActionDispatcher();

    bool initialize();
    ActionDispatchResult process(const GesturePipelineResult& pipelineResult);
    bool releaseAll();
    bool setInputEnabled(bool enabled);
    bool applyConfiguration(PointerMappingConfig mappingConfig,
                            InputConfig inputConfig);
    void shutdown();

    bool inputEnabled() const { return m_inputEnabled; }
    bool buttonDown() const { return m_buttonDown; }
    Handedness activeHand() const { return m_activeHand; }
    const PointerMappingConfig& pointerConfig() const {
        return m_pointerMapper.config();
    }
    const InputConfig& inputConfig() const { return m_inputConfig; }
    const std::string& lastError() const { return m_lastError; }

private:
    struct FrameMetadata {
        bool present{false};
        std::uint64_t frameId{0};
        std::int64_t timestampUs{0};
    };

    const GestureObservation* selectActiveObservation(
        const GesturePipelineResult& pipelineResult) const;
    static FrameMetadata metadata(const GesturePipelineResult& pipelineResult);
    ActionCommand command(ActionType type, const FrameMetadata& metadata,
                          Handedness hand, DesktopPoint point = {}) const;
    bool dispatchButtonDown(const ActionCommand& command,
                            ActionDispatchResult& result);
    bool dispatchButtonUp(const ActionCommand& command,
                          ActionDispatchResult& result);
    bool dispatchMove(const ActionCommand& command,
                      ActionDispatchResult& result);
    void fail(ActionDispatchResult& result, const std::string& error);

    ISystemInputBackend& m_backend;
    PointerMapper m_pointerMapper;
    InputConfig m_inputConfig;
    DesktopGeometry m_desktop{};
    Handedness m_activeHand{Handedness::UNKNOWN};
    Handedness m_buttonHand{Handedness::UNKNOWN};
    std::int64_t m_lastTimestampUs{0};
    bool m_initialized{false};
    bool m_inputEnabled{false};
    bool m_buttonDown{false};
    bool m_hasTimestamp{false};
    std::string m_lastError;
};

#endif // ACTIONDISPATCHER_H
