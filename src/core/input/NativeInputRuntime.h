#ifndef NATIVEINPUTRUNTIME_H
#define NATIVEINPUTRUNTIME_H

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

#include "src/core/actions/ISystemInputBackend.h"
#include "src/core/actions/PointerMapper.h"
#include "src/core/config/InputConfig.h"
#include "src/core/gestures/GesturePipeline.h"
#include "src/core/input/NativeInputState.h"

class NativeInputRuntime final {
public:
    static constexpr std::size_t kMaxPendingSemanticFrames = 16;

    explicit NativeInputRuntime(
        std::unique_ptr<ISystemInputBackend> backend,
        PointerMappingConfig pointerConfig = {},
        InputConfig inputConfig = {});
    ~NativeInputRuntime();

    NativeInputRuntime(const NativeInputRuntime&) = delete;
    NativeInputRuntime& operator=(const NativeInputRuntime&) = delete;

    void requestConfiguration(PointerMappingConfig pointerConfig,
                              InputConfig inputConfig);
    void requestEnabled(bool enabled);
    void submitLatest(const GesturePipelineResult& result);

    NativeInputStatus status() const;
    bool waitForState(NativeInputState expected,
                      std::chrono::milliseconds timeout) const;
    void shutdown();

private:
    static bool containsRelease(const GesturePipelineResult& result,
                                Handedness hand = Handedness::UNKNOWN);
    void requestEnabledLocked(bool enabled);
    void enqueueSemanticLocked(const GesturePipelineResult& result);
    void clearPendingLocked();
    void publishStateLocked(NativeInputState state,
                            std::string error = {});
    void workerMain(std::stop_token stopToken,
                    std::unique_ptr<ISystemInputBackend> backend);

    mutable std::mutex m_mutex;
    mutable std::condition_variable m_condition;
    NativeInputState m_state{NativeInputState::DISABLED};
    bool m_desiredEnabled{false};
    bool m_configurationDirty{false};
    bool m_shutdownRequested{false};
    std::uint64_t m_generation{0};
    PointerMappingConfig m_pointerConfig;
    InputConfig m_inputConfig;
    std::string m_error;
    std::optional<GesturePipelineResult> m_latestFrame;
    std::deque<GesturePipelineResult> m_semanticFrames;
    std::uint64_t m_droppedFrameCount{0};
    std::uint64_t m_droppedSemanticFrameCount{0};
    std::jthread m_worker;
};

#endif // NATIVEINPUTRUNTIME_H
