# Real inference validation

## Pinned source

- MediaPipe `v0.10.26`
- Commit/tag target: `80ae8afbd03465b0d6d9f9e874f8cacf093d23e9`
- Acquisition: official codeload tag tarball
- Tarball SHA-256: `51cbd4d538716e3722b55b8412cab9e3713270a6e9a6f09af6b52d357718e6d7`

## Bridge build

Bazelisk selected Bazel 6.5.0 from the pinned checkout. Required environment/build inputs were hermetic Python 3.12, EGL/GLES development headers, and system OpenCV 4.6 exposed through MediaPipe's `linux_opencv` rule. Command:

```sh
bazel build -c opt --repo_env=HERMETIC_PYTHON_VERSION=3.12 \
  //c0ntrol_bridge:libc0ntrol_mediapipe_bridge.so
```

Result: **PASS**. Artifact: `/tmp/mediapipe-src/bazel-bin/c0ntrol_bridge/libc0ntrol_mediapipe_bridge.so`, 23,604,176 bytes. `ldd` resolved its dependencies.

## Model and inference

- Model: `models/hand_landmarker.task`, 7,819,105 bytes.
- `HandLandmarker::Create`: **PASS**, proven by both integration executables.
- Neutral generated RGB inference: **PASS** twice, including `timestampUs=1000` then `1001`, shutdown twice.
- Real-hand asset: `tests/assets/hand_test.ppm`, SHA-256 `c470119d7285ac44785cbb152510418f2653a98050e1f0c283598145c9ee4daf`.
- Origin: lossless PPM conversion of MediaPipe's official Apache-2.0 `pointing_up.jpg` test asset; no random external image.
- Real inference observation: `hands=1 handedness=RIGHT score=0.995122 world=21`.
- Domain result: valid `HandTrackingFrame`; one hand; 21 finite, non-zero normalized landmarks; RIGHT handedness; score in [0,1]; 21 finite world landmarks.

## Test results

- Core/headless: **6/6 PASS**; `make test` PASS.
- MediaPipe integration: **2/2 PASS**, total 1.62 s.
- Desktop application build with MediaPipe enabled: **PASS** after Qt6/OpenCV environment provisioning and a metatype include-order compile fix.
- Camera runtime: **NOT AVAILABLE** (`/dev/video*` absent).
- Full live shutdown runtime: **NOT TESTED**; backend idempotent shutdown is exercised by integration tests.
