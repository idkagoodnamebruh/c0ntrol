#include "MediaPipeHandTrackingBackend.h"

#include <array>
#include <filesystem>
#include <limits>

#include "third_party/mediapipe_bridge/c0ntrol_mediapipe_bridge.h"

class MediaPipeHandTrackingBackend::Impl {
public:
    C0ntrolMpHandle* handle{nullptr};
    std::string error;
    std::int64_t lastTimestampMs{-1};
};

MediaPipeHandTrackingBackend::MediaPipeHandTrackingBackend() : m_impl(std::make_unique<Impl>()) {}
MediaPipeHandTrackingBackend::~MediaPipeHandTrackingBackend() { shutdown(); }

bool MediaPipeHandTrackingBackend::initialize(const HandTrackingConfig& config) {
    shutdown();
    std::error_code ec;
    if (!std::filesystem::is_regular_file(config.modelPath, ec) || ec ||
        std::filesystem::file_size(config.modelPath, ec) == 0 || ec) {
        m_impl->error = "Hand Landmarker model is missing, unreadable, or empty: " + config.modelPath;
        return false;
    }
    std::array<char, C0NTROL_MP_ERROR_SIZE> error{};
    m_impl->handle = c0ntrol_mp_create(
        config.modelPath.c_str(), config.numHands,
        config.minHandDetectionConfidence, config.minHandPresenceConfidence,
        config.minTrackingConfidence, error.data(), error.size());
    m_impl->error = error.data();
    m_impl->lastTimestampMs = -1;
    return m_impl->handle != nullptr;
}

HandTrackingFrame MediaPipeHandTrackingBackend::process(
    const RgbImageView& image, std::int64_t timestampUs, std::uint64_t frameId) {
    HandTrackingFrame output;
    output.timestampUs = timestampUs;
    output.frameId = frameId;
    if (!m_impl->handle || !image.isValid()) {
        m_impl->error = m_impl->handle ? "Invalid RGB frame" : "MediaPipe backend is not initialized";
        return output;
    }
    auto timestampMs = timestampUs / 1000;
    if (timestampMs <= m_impl->lastTimestampMs) timestampMs = m_impl->lastTimestampMs + 1;
    m_impl->lastTimestampMs = timestampMs;

    C0ntrolMpResult result{};
    std::array<char, C0NTROL_MP_ERROR_SIZE> error{};
    if (!c0ntrol_mp_detect_video(m_impl->handle, image.data, image.width,
                                 image.height, static_cast<int>(image.rowStride),
                                 timestampMs, &result, error.data(), error.size())) {
        m_impl->error = error.data();
        return output;
    }
    for (int h = 0; h < result.hand_count; ++h) {
        TrackedHand hand;
        for (std::size_t i = 0; i < hand.landmarks.size(); ++i) {
            const auto& point = result.hands[h].normalized[i];
            hand.landmarks[i] = Point3D(point.x, point.y, point.z);
        }
        if (result.hands[h].has_world) {
            std::array<Point3D, 21> world{};
            for (std::size_t i = 0; i < world.size(); ++i) {
                const auto& point = result.hands[h].world[i];
                world[i] = Point3D(point.x, point.y, point.z);
            }
            hand.worldLandmarks = world;
        }
        hand.handednessScore = result.hands[h].handedness_score;
        if (result.hands[h].handedness == 1) hand.handedness = Handedness::LEFT;
        else if (result.hands[h].handedness == 2) hand.handedness = Handedness::RIGHT;
        output.hands.push_back(std::move(hand));
    }
    output.valid = true;
    m_impl->error.clear();
    return output;
}

void MediaPipeHandTrackingBackend::shutdown() {
    if (!m_impl->handle) return;
    std::array<char, C0NTROL_MP_ERROR_SIZE> error{};
    if (!c0ntrol_mp_close(m_impl->handle, error.data(), error.size())) m_impl->error = error.data();
    m_impl->handle = nullptr;
    m_impl->lastTimestampMs = -1;
}

std::string MediaPipeHandTrackingBackend::lastError() const { return m_impl->error; }
