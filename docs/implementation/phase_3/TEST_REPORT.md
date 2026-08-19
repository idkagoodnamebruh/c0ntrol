# Phase 3 test report

## Environment

- Host path: Windows workspace compiled through Arch Linux WSL.
- Compiler: GNU C++ 16.2.1.
- CMake: 4.4.2.
- Qt: 6.11.1.
- OpenCV: 5.0.0.
- Approved base: `6535e2248b540c175bb5b22e958cce6533360500`.

## Clean core suite

Commands:

```text
cmake -S . -B build-tests -DBUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --parallel 4
ctest --test-dir build-tests --output-on-failure
```

Result: **7/7 PASS**.

| Target | Result |
|---|---|
| `test_display_transform` | PASS |
| `test_dynamic_gestures` | PASS |
| `test_frame_sync` | PASS |
| `test_hand_geometry` | PASS |
| `test_one_euro` | PASS |
| `test_landmark_filter_bank` | PASS |
| `test_hand_tracking` | PASS |

The six pre-existing Phase 2 targets remain **6/6 PASS**. The two filtering-focused targets are **2/2 PASS**. `make test` independently configured, built, and ran the same suite: **7/7 PASS**.

`test_one_euro` covers first/constant samples, deterministic jitter reduction, progressive step response, higher-beta response, active derivative cutoff, irregular time, repeated/regressive/large-gap/non-finite timestamps, reset, and NaN/Inf recovery.

`test_landmark_filter_bank` covers stable LEFT/RIGHT, two hands, order swap, X/Y/Z and landmark independence, UNKNOWN, duplicates, empty/invalid frames, timeout, reappearance, teleport, repeated time, world present/absent and space isolation, metadata/raw preservation, runtime passthrough, jitter reduction, and adaptive motion.

## Desktop and MediaPipe

Command:

```text
cmake -S . -B build-app-phase3 -DBUILD_APP=ON -DBUILD_TESTING=OFF -DENABLE_MEDIAPIPE=OFF
cmake --build build-app-phase3 --parallel 4
```

Result: **PASS**; `c0ntrol` compiled and linked, including Qt MOC, `VisionWorker.cpp`, and `LandmarkFilterBank.cpp`.

No `libc0ntrol_mediapipe_bridge.so` exists under `/tmp`, `/opt`, or the accessible Codex workspaces, so the two Phase 2 MediaPipe tests are **NOT RE-RUN / EXTERNAL ARTIFACT UNAVAILABLE**. No backend/model/bridge source was changed, and the approved Phase 2 result remains 2/2 PASS evidence rather than a new Phase 3 run.

No `/dev/video*` device is exposed in WSL, so camera tuning is **NOT AVAILABLE** and parameter values remain initial defaults.
