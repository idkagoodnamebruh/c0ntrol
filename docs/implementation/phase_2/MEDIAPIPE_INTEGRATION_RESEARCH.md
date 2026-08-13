# MediaPipe integration research

## Official sources consulted

The research scope was restricted to the official [Google AI Hand Landmarker guide](https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker), the [MediaPipe repository](https://github.com/google-ai-edge/mediapipe), its [C++ Hand Landmarker header](https://github.com/google-ai-edge/mediapipe/blob/master/mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h), [options header](https://github.com/google-ai-edge/mediapipe/blob/master/mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker_options.h), and [result header](https://github.com/google-ai-edge/mediapipe/blob/master/mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker_result.h). Web retrieval from this sandbox returned HTTP 401/403, so exact upstream revision/build compatibility could not be verified here; links identify the authoritative material that must be pinned before production.

## API decision

The implementation targets the native **C++ Tasks API** in `mediapipe/tasks/cc/vision/hand_landmarker`. Ownership is RAII: `HandLandmarker::Create(options)` returns a status-or unique owner; destruction/shutdown releases it. Options include model asset path, running mode, `num_hands`, detection/presence/tracking confidence thresholds and (for live stream) a result callback.

The result contract supplies per-hand normalized landmarks, world landmarks and categorized handedness with score. Results are copied immediately to c0ntrol domain types; no MediaPipe object escapes the backend.

## VIDEO versus LIVE_STREAM

**VIDEO was selected temporarily.** Each QTimer timeout calls `DetectForVideo` once and returns. It avoids callback/result/image lifetime ambiguity while establishing the boundary, but inference can block that worker iteration and latency is not yet measured. Timestamps passed to MediaPipe are monotonic milliseconds derived from the domain microsecond timestamp.

LIVE_STREAM is the intended later optimization. Its callback executes according to MediaPipe's task runner, input images must outlive asynchronous consumption, late callbacks must be rejected during shutdown, results need frame correlation, and the task may ignore/drop submissions while busy. Those policies require an owned in-flight buffer and explicit backpressure, not a borrowed `cv::Mat` view.

## Images and ownership

OpenCV converts BGR to RGB. The backend allocates an owned `mediapipe::ImageFrame` and copies each row respecting source stride and destination `WidthStep`; therefore MediaPipe never retains a pointer into the temporary OpenCV buffer. Expected format is SRGB (three channels).

## Build boundary

MediaPipe is normally built with its upstream Bazel graph. This repository remains CMake-based and expects a separately version-pinned Hand Landmarker library plus official include tree through an imported target. No source vendoring or implicit download occurs. The exact upstream revision, transitive link closure and produced artifact remain environment-blocked and must be validated before enabling the option.
