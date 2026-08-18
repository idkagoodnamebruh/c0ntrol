# Build integration

Pinned dependency: MediaPipe `v0.10.26`, commit `80ae8afbd03465b0d6d9f9e874f8cacf093d23e9`.

## Reproducible Bazel → CMake boundary

The official Hand Landmarker is a `cc_library`, not a standalone shared library. Phase 2B therefore supplies `third_party/mediapipe_bridge`: a small C ABI and Bazel `cc_binary(linkshared=True)` whose dependency on the official target lets Bazel resolve/link the transitive closure.

```sh
git clone --branch v0.10.26 --depth 1 https://github.com/google-ai-edge/mediapipe.git
cd mediapipe
test "$(git rev-parse HEAD)" = 80ae8afbd03465b0d6d9f9e874f8cacf093d23e9
cp -R /path/to/c0ntrol/third_party/mediapipe_bridge c0ntrol_bridge
bazel build -c opt //c0ntrol_bridge:libc0ntrol_mediapipe_bridge.so
```

The real artifact is `bazel-bin/c0ntrol_bridge/libc0ntrol_mediapipe_bridge.so`. Configure the headless integration target independently of Qt/OpenCV:

```sh
cmake -S /path/to/c0ntrol -B build-mediapipe \
  -DBUILD_APP=OFF -DBUILD_TESTING=ON -DENABLE_MEDIAPIPE=ON \
  -DMEDIAPIPE_BRIDGE_LIBRARY=/path/to/mediapipe/bazel-bin/c0ntrol_bridge/libc0ntrol_mediapipe_bridge.so
cmake --build build-mediapipe --target test_mediapipe_hand_tracking_backend
ctest --test-dir build-mediapipe -R mediapipe --output-on-failure
```

The imported CMake target points only to that Bazel-produced closure and the stable bridge header. No hypothetical `libhand_landmarker.so`, source vendoring, download-at-configure, or Qt dependency is involved.

Validated command adds `--repo_env=HERMETIC_PYTHON_VERSION=3.12`. On Ubuntu 24.04 the pinned upstream build also required EGL/GLES and OpenCV development packages; MediaPipe's `third_party/opencv_linux.BUILD` was configured to expose `/usr/include/opencv4` in the temporary upstream checkout. The project bridge BUILD needed its header in `cc_binary.srcs` because `cc_binary` has no `hdrs` attribute.

Runtime asset `models/hand_landmarker.task` is 7,819,105 bytes and is a readable Task ZIP containing `hand_detector.tflite` and `hand_landmarks_detector.tflite`. `HandLandmarker::Create` and inference both passed through the real bridge. Upstream license is Apache-2.0; distributions must preserve notices.
