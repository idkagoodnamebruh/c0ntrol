#include "MediaPipeHandTrackingBackend.h"

#include <algorithm>
#include <filesystem>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/core/base_options.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h"

namespace mp = mediapipe;
namespace tasks = mediapipe::tasks;
namespace vision = mediapipe::tasks::vision;
namespace hand_landmarker = mediapipe::tasks::vision::hand_landmarker;

class MediaPipeHandTrackingBackend::Impl {
public:
    std::unique_ptr<hand_landmarker::HandLandmarker> landmarker;
    std::string error;
};

MediaPipeHandTrackingBackend::MediaPipeHandTrackingBackend() : m_impl(std::make_unique<Impl>()) {}
MediaPipeHandTrackingBackend::~MediaPipeHandTrackingBackend() = default;

bool MediaPipeHandTrackingBackend::initialize(const HandTrackingConfig& config) {
    shutdown();
    if (!std::filesystem::is_regular_file(config.modelPath) ||
        std::filesystem::file_size(config.modelPath) == 0) {
        m_impl->error = "Hand Landmarker model is missing or empty: " + config.modelPath;
        return false;
    }

    auto options = std::make_unique<hand_landmarker::HandLandmarkerOptions>();
    options->base_options.model_asset_path = config.modelPath;
    options->running_mode = vision::core::RunningMode::VIDEO;
    options->num_hands = config.numHands;
    options->min_hand_detection_confidence = config.minHandDetectionConfidence;
    options->min_hand_presence_confidence = config.minHandPresenceConfidence;
    options->min_tracking_confidence = config.minTrackingConfidence;
    auto created = hand_landmarker::HandLandmarker::Create(std::move(options));
    if (!created.ok()) {
        m_impl->error = created.status().ToString();
        return false;
    }
    m_impl->landmarker = std::move(created.value());
    m_impl->error.clear();
    return true;
}

HandTrackingFrame MediaPipeHandTrackingBackend::process(const RgbImageView& image,
                                                         std::int64_t timestampUs,
                                                         std::uint64_t frameId) {
    HandTrackingFrame output;
    output.timestampUs = timestampUs;
    output.frameId = frameId;
    if (!m_impl->landmarker || !image.isValid()) {
        m_impl->error = m_impl->landmarker ? "Invalid RGB frame" : "MediaPipe backend is not initialized";
        return output;
    }

    auto frame = std::make_shared<mp::ImageFrame>(mp::ImageFormat::SRGB, image.width,
                                                  image.height,
                                                  mp::ImageFrame::kDefaultAlignmentBoundary);
    for (int row = 0; row < image.height; ++row) {
        std::copy_n(image.data + row * image.rowStride, image.width * 3,
                    frame->MutablePixelData() + row * frame->WidthStep());
    }
    mp::Image mpImage(frame);
    const auto result = m_impl->landmarker->DetectForVideo(mpImage, timestampUs / 1000);
    if (!result.ok()) {
        m_impl->error = result.status().ToString();
        return output;
    }

    const auto& value = result.value();
    output.hands.reserve(value.hand_landmarks.size());
    for (std::size_t h = 0; h < value.hand_landmarks.size(); ++h) {
        if (value.hand_landmarks[h].size() != 21) continue;
        TrackedHand hand;
        for (std::size_t i = 0; i < 21; ++i) {
            const auto& point = value.hand_landmarks[h][i];
            hand.landmarks[i] = Point3D(point.x, point.y, point.z);
        }
        if (h < value.hand_world_landmarks.size() &&
            value.hand_world_landmarks[h].size() == 21) {
            std::array<Point3D, 21> world{};
            for (std::size_t i = 0; i < 21; ++i) {
                const auto& point = value.hand_world_landmarks[h][i];
                world[i] = Point3D(point.x, point.y, point.z);
            }
            hand.worldLandmarks = world;
        }
        if (h < value.handedness.size() && !value.handedness[h].empty()) {
            const auto& category = value.handedness[h].front();
            hand.handednessScore = category.score;
            if (category.category_name == "Left") hand.handedness = Handedness::LEFT;
            else if (category.category_name == "Right") hand.handedness = Handedness::RIGHT;
        }
        output.hands.push_back(std::move(hand));
    }
    output.valid = true; // A valid inference may legitimately contain zero hands.
    m_impl->error.clear();
    return output;
}

void MediaPipeHandTrackingBackend::shutdown() { m_impl->landmarker.reset(); }
std::string MediaPipeHandTrackingBackend::lastError() const { return m_impl->error; }
