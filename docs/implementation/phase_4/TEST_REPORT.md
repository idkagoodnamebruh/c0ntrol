# Phase 4 test report

## Environment

- Base: `8d6fca4810a41854fbe100949717d2d2b6964481`.
- Compiler: GCC 16.2.1 in Arch Linux WSL.
- Desktop dependencies: Qt 6 and OpenCV 5.0.0 found.
- Camera devices: none exposed under `/dev/video*`.
- Prior MediaPipe bridge artifact: not present; MediaPipe source and backend were
  unchanged, so real inference was not repeated.

## Core regression

The initial `build-tests` directory did not exist. The requested clean configure,
build and CTest sequence completed with 10/10 PASS:

1. test_display_transform
2. test_dynamic_gestures
3. test_frame_sync
4. test_hand_geometry
5. test_hand_features
6. test_gesture_state_machine
7. test_gesture_pipeline
8. test_one_euro
9. test_landmark_filter_bank
10. test_hand_tracking

The seven Phase 3 tests remain 7/7 PASS. The three new gesture targets are 3/3
PASS. A separate `make test` run also completed 10/10 PASS.

## New coverage

`test_hand_features` validates finite/non-degenerate hand scale, normalized
pinch ratio at scales 0.5/1/2, static pose preservation under scale, finger
extension/curl, mirrored LEFT/RIGHT thumb behavior, rotations 0/45/90 degrees,
NaN, Inf and zero geometry.

`test_gesture_state_machine` validates the 11 required behaviors: stable idle,
single-frame false entry, sustained entry, a one-second hold without repeated
BEGIN, hysteresis band, short release, confirmed release, re-pinch, safe loss,
timestamp anomalies and UNKNOWN/mismatched handedness isolation.

`test_gesture_pipeline` validates the exact ordered sequence OPEN_HAND,
POINTING/POINTER_ACTIVE, candidate, PINCH_BEGIN, hold, PINCH_END, POINTING. It
also validates UNKNOWN isolation and deterministic independent LEFT/RIGHT
events.

## Desktop

Configuration with `BUILD_APP=ON`, `BUILD_TESTING=OFF`, and
`ENABLE_MEDIAPIPE=OFF` compiled and linked successfully. No application or
camera interaction was claimed because no camera device was available.
