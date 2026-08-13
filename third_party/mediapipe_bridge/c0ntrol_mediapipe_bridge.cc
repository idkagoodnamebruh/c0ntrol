#include "c0ntrol_mediapipe_bridge.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include "mediapipe/framework/formats/image.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/tasks/cc/vision/core/running_mode.h"
#include "mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h"

namespace hl = mediapipe::tasks::vision::hand_landmarker;
namespace vc = mediapipe::tasks::vision::core;

struct C0ntrolMpHandle { std::unique_ptr<hl::HandLandmarker> landmarker; };

static void SetError(char* output, int size, const std::string& message) {
  if (!output || size <= 0) return;
  std::strncpy(output, message.c_str(), static_cast<std::size_t>(size - 1));
  output[size - 1] = '\0';
}

extern "C" C0ntrolMpHandle* c0ntrol_mp_create(
    const char* model_path, int num_hands, float detection_confidence,
    float presence_confidence, float tracking_confidence,
    char* error, int error_size) {
  if (!model_path) { SetError(error, error_size, "model_path is null"); return nullptr; }
  auto options = std::make_unique<hl::HandLandmarkerOptions>();
  options->base_options.model_asset_path = model_path;
  options->running_mode = vc::RunningMode::VIDEO;
  options->num_hands = std::clamp(num_hands, 1, C0NTROL_MP_MAX_HANDS);
  options->min_hand_detection_confidence = detection_confidence;
  options->min_hand_presence_confidence = presence_confidence;
  options->min_tracking_confidence = tracking_confidence;
  auto created = hl::HandLandmarker::Create(std::move(options));
  if (!created.ok()) { SetError(error, error_size, created.status().ToString()); return nullptr; }
  auto* handle = new C0ntrolMpHandle;
  handle->landmarker = std::move(created.value());
  SetError(error, error_size, "");
  return handle;
}

extern "C" int c0ntrol_mp_detect_video(
    C0ntrolMpHandle* handle, const uint8_t* rgb, int width, int height,
    int stride, int64_t timestamp_ms, C0ntrolMpResult* output,
    char* error, int error_size) {
  if (!handle || !handle->landmarker || !rgb || !output || width <= 0 ||
      height <= 0 || stride < width * 3) {
    SetError(error, error_size, "invalid detect arguments"); return 0;
  }
  *output = {};
  auto frame = std::make_shared<mediapipe::ImageFrame>(
      mediapipe::ImageFormat::SRGB, width, height,
      mediapipe::ImageFrame::kDefaultAlignmentBoundary);
  for (int row = 0; row < height; ++row) {
    std::copy_n(rgb + row * stride, width * 3,
                frame->MutablePixelData() + row * frame->WidthStep());
  }
  auto detected = handle->landmarker->DetectForVideo(mediapipe::Image(frame), timestamp_ms);
  if (!detected.ok()) { SetError(error, error_size, detected.status().ToString()); return 0; }
  const auto& value = detected.value();
  const auto count = std::min(value.hand_landmarks.size(),
                              static_cast<std::size_t>(C0NTROL_MP_MAX_HANDS));
  for (std::size_t h = 0; h < count; ++h) {
    const auto& normalized = value.hand_landmarks[h].landmarks;
    if (normalized.size() != C0NTROL_MP_LANDMARKS) continue;
    auto& hand = output->hands[output->hand_count];
    for (std::size_t i = 0; i < normalized.size(); ++i)
      hand.normalized[i] = {normalized[i].x, normalized[i].y, normalized[i].z};
    if (h < value.hand_world_landmarks.size()) {
      const auto& world = value.hand_world_landmarks[h].landmarks;
      if (world.size() == C0NTROL_MP_LANDMARKS) {
        hand.has_world = 1;
        for (std::size_t i = 0; i < world.size(); ++i)
          hand.world[i] = {world[i].x, world[i].y, world[i].z};
      }
    }
    if (h < value.handedness.size()) {
      const auto& categories = value.handedness[h].categories;
      if (!categories.empty()) {
        const auto& category = categories.front();
        hand.handedness_score = category.score;
        if (category.category_name) {
          if (*category.category_name == "Left") hand.handedness = 1;
          else if (*category.category_name == "Right") hand.handedness = 2;
        }
      }
    }
    ++output->hand_count;
  }
  SetError(error, error_size, "");
  return 1;
}

extern "C" int c0ntrol_mp_close(C0ntrolMpHandle* handle, char* error, int error_size) {
  if (!handle) return 1;
  if (!handle->landmarker) { delete handle; return 1; }
  const auto status = handle->landmarker->Close();
  delete handle;
  if (!status.ok()) { SetError(error, error_size, status.ToString()); return 0; }
  SetError(error, error_size, "");
  return 1;
}
