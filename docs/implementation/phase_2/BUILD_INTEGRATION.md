# Build integration

`ENABLE_MEDIAPIPE` defaults OFF. Headless tests therefore need neither Qt/OpenCV nor MediaPipe. With it ON, CMake requires `MEDIAPIPE_ROOT` and `MEDIAPIPE_HAND_LANDMARKER_LIBRARY`, creates imported target `MediaPipe::HandLandmarker`, compiles the concrete backend, defines `C0NTROL_ENABLE_MEDIAPIPE`, and links it to the app.

Expected workflow after building an officially pinned MediaPipe revision with Bazel:

```sh
cmake -S . -B build -DBUILD_APP=ON -DENABLE_MEDIAPIPE=ON \
  -DMEDIAPIPE_ROOT=/path/to/mediapipe \
  -DMEDIAPIPE_HAND_LANDMARKER_LIBRARY=/path/to/libhand_landmarker.so
cmake --build build
```

The dependency revision and full transitive native-library closure could not be established in this environment. MediaPipe is Apache-2.0 upstream; packaging must retain applicable notices. Runtime asset is `models/hand_landmarker.task`: 7,819,105 bytes, readable ZIP Task bundle containing `hand_detector.tflite` and `hand_landmarks_detector.tflite`. No download script or model changed.
