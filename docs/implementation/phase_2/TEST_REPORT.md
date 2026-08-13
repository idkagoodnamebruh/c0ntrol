# Phase 2 test report

Environment: Ubuntu 24.04 x86_64, GNU C++ 13.3, CMake 3.28. Qt6/OpenCV/MediaPipe development artifacts unavailable.

Clean headless configure/build/CTest: **6/6 PASS** (five Phase 1 tests plus `test_hand_tracking`), total 0.08 s. `make test`: **6/6 PASS**, total 0.08 s. The new test covers defaults, fixed 21-point shape, handedness, zero/two hands and RIGHT preference, IDs/timestamps, mock success and uninitialized failure.

Application configure with MediaPipe OFF: **BLOCKED BY ENVIRONMENT** at missing Qt6. The Phase 2B MediaPipe-only target is independent from the GUI, but its pinned Bazel bridge artifact was unavailable: backend compile/link, model `Create`, generated-static-image inference and conversion behavior are **BLOCKED**. Camera and shutdown runtime are **NOT TESTED**. The generated neutral image deliberately makes no hand-detection claim.

Bazel 9.2.0 was available, but cloning the pinned official repository failed with `HTTP 403` (`fatal: expected flush after ref listing`). MediaPipe-enabled CMake correctly failed early without `MEDIAPIPE_BRIDGE_LIBRARY`. Consequently no actual MediaPipe compile/link claim is made.

As a limited boundary check, a temporary fake implementation of the project-owned C ABI allowed CMake to compile and link `MediaPipeHandTrackingBackend.cpp` plus the integration-test executable successfully. This validates the CMake isolation and consumer-side ABI only; it does **not** validate the Bazel bridge, official MediaPipe linkage, model creation, or inference.
