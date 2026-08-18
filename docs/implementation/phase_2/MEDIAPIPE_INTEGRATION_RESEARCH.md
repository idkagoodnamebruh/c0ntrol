# MediaPipe integration research

## Pinned upstream

- `MEDIAPIPE_VERSION=v0.10.26`
- `MEDIAPIPE_COMMIT_SHA=80ae8afbd03465b0d6d9f9e874f8cacf093d23e9`

The SHA is the official GitHub tag target returned for `refs/tags/v0.10.26`. Phase 2B targets this revision rather than `master`.

## Official sources consulted

The research scope was restricted to the official [Google AI Hand Landmarker guide](https://ai.google.dev/edge/mediapipe/solutions/vision/hand_landmarker), the [v0.10.26 C++ Hand Landmarker header](https://github.com/google-ai-edge/mediapipe/blob/v0.10.26/mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker.h), [result header](https://github.com/google-ai-edge/mediapipe/blob/v0.10.26/mediapipe/tasks/cc/vision/hand_landmarker/hand_landmarker_result.h), [landmark containers](https://github.com/google-ai-edge/mediapipe/blob/v0.10.26/mediapipe/tasks/cc/components/containers/landmark.h), [classification containers](https://github.com/google-ai-edge/mediapipe/blob/v0.10.26/mediapipe/tasks/cc/components/containers/classification_result.h), and [Category](https://github.com/google-ai-edge/mediapipe/blob/v0.10.26/mediapipe/tasks/cc/components/containers/category.h). Raw official headers and the GitHub tag API were reachable for verification.

## API decision

The pinned implementation targets the native **C++ Tasks API** behind a stable project-owned C ABI. `HandLandmarker::Create(options)` returns a status-or unique owner and `Close()` is called explicitly before destruction. `NormalizedLandmarks.landmarks`, `Landmarks.landmarks`, `Classifications.categories`, and optional `Category::category_name` are handled according to the pinned headers.

The result contract supplies per-hand normalized landmarks, world landmarks and categorized handedness with score. Results are copied immediately to c0ntrol domain types; no MediaPipe object escapes the backend.

## VIDEO versus LIVE_STREAM

**VIDEO was selected temporarily.** Each QTimer timeout calls `DetectForVideo` once and returns. It avoids callback/result/image lifetime ambiguity while establishing the boundary, but inference can block that worker iteration and latency is not yet measured. Timestamps passed to MediaPipe are monotonic milliseconds derived from the domain microsecond timestamp.

LIVE_STREAM is the intended later optimization. Its callback executes according to MediaPipe's task runner, input images must outlive asynchronous consumption, late callbacks must be rejected during shutdown, results need frame correlation, and the task may ignore/drop submissions while busy. Those policies require an owned in-flight buffer and explicit backpressure, not a borrowed `cv::Mat` view.

## Images and ownership

OpenCV converts BGR to RGB. The pinned Bazel bridge allocates an owned `mediapipe::ImageFrame` and copies each row respecting source stride and destination `WidthStep`; therefore MediaPipe never retains a pointer into the temporary OpenCV buffer. Expected format is SRGB (three channels).

## Build boundary

MediaPipe is built with its upstream Bazel graph. This repository remains CMake-based and consumes only the project-owned shared C bridge produced by Bazel; that target depends on the official Hand Landmarker target so Bazel owns the transitive closure. CMake imports the concrete bridge artifact and needs no MediaPipe include tree. No source vendoring or implicit download occurs.

Phase 2C validated this boundary using the official tag tarball. Bazelisk selected the checkout's Bazel 6.5.0, and the bridge built after supplying hermetic Python 3.12 plus the EGL/GLES and OpenCV development inputs expected by upstream. No dependency revision changed.
