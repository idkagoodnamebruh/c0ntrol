#ifndef MEDIAPIPEHANDTRACKINGBACKEND_H
#define MEDIAPIPEHANDTRACKINGBACKEND_H

#include <memory>

#include "src/core/tracking/IHandTrackingBackend.h"

class MediaPipeHandTrackingBackend final : public IHandTrackingBackend {
public:
    MediaPipeHandTrackingBackend();
    ~MediaPipeHandTrackingBackend() override;
    bool initialize(const HandTrackingConfig& config) override;
    HandTrackingFrame process(const RgbImageView& image, std::int64_t timestampUs,
                              std::uint64_t frameId) override;
    void shutdown() override;
    std::string lastError() const override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // MEDIAPIPEHANDTRACKINGBACKEND_H
