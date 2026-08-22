# Phase 6 test report

## Linux core

Clean configuration:

```text
cmake -S . -B /tmp/c0ntrol-phase6-core \
  -DBUILD_APP=OFF -DBUILD_TESTING=ON -DENABLE_MEDIAPIPE=OFF
cmake --build /tmp/c0ntrol-phase6-core --parallel
ctest --test-dir /tmp/c0ntrol-phase6-core --output-on-failure
```

Environment: WSL, GCC 16.2.1, CMake 4.4.2. Result: **16/16 PASS**.
The 13 pre-Phase-6 Linux targets remain green and the three new targets pass:

- `test_latest_frame_slot`
- `test_async_capture`
- `test_pipeline_metrics`

Each new target was also run 20 consecutive times with CTest
`--repeat until-fail:20`; all runs passed.

## Phase 6 cases

The tests prove an empty slot, single publish/consume, `1,2,3 -> 3`, exact
overwrite counting, preserved sequence, no duplicate consume and concurrent
producer/consumer safety. The async suite proves start, finite completion,
restart, fatal source failure, cancelable blocking stop, strict capture
timestamps and a 250-frame fast-producer/slow-consumer case. That stress case
captured all 250 synthetic frames, observed overwrites, processed fewer than
250, and converged on capture sequence 250 without a queue.

Metrics tests use deterministic timestamps to validate bounded rate samples,
captured/processed/overwrite counts and nonnegative age/inference/processing
durations.

## Windows CI

- Workflow: `Phase 5 Windows Build` (the existing cross-platform regression).
- Implementation run: `32560820110`, job `97001969422`.
- Validated implementation commit: `d82ae5d6940491a234c5d8ac99bdb7f2c92f117a`.
- Runner: `windows-latest`.
- CMake: 4.4.2.
- Compiler: MSVC 19.51.36256.0.
- MSBuild: 18.9.1+a81b43525.
- Configure/build/test: PASS.
- Windows CTest: **17/17 PASS** in 0.56 seconds.
- New Phase 6 targets: **3/3 PASS**.
- Existing `test_windows_input_backend_compile`: PASS; its executable was
  compiled and linked, preserving the Phase 5 Windows backend boundary.

The workflow does not build the Qt/OpenCV desktop application and does not
execute native `SendInput`, consistent with its established scope.

## Linux desktop integration

Clean `BUILD_APP=ON`, `BUILD_TESTING=OFF`, `ENABLE_MEDIAPIPE=OFF` configuration
found OpenCV 5.0.0 and Qt, compiled `VisionWorker.cpp` and
`OpenCVCameraSource.cpp`, and linked the `c0ntrol` executable: **PASS**.

No `/dev/video*` device was exposed in WSL. Real camera open/read/restart and
driver-level blocking shutdown are therefore **NOT AVAILABLE**, not presented
as passes. MediaPipe was not rerun because its backend, bridge and model were
not modified.
