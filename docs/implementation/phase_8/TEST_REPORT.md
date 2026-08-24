# Phase 8 test report

## Linux core

Clean-equivalent WSL configuration used GCC 16.2.1 and CMake 4.4.2 with
`BUILD_APP=OFF`, `BUILD_TESTING=ON` and `ENABLE_MEDIAPIPE=OFF`. Build and CTest
completed **19/19 PASS**. The same 19 Phase 7 targets remain present; Phase 8
replaced and expanded the legacy dynamic test rather than inflating the target
count.

Dynamic coverage includes stationary/small/slow motion; four directions;
ambiguous diagonal; hand scales 0.5x/1x/2x; irregular timestamp intervals;
POINTING and PINCH gates; cooldown and post-cooldown reuse; tracking loss;
sample gap; repeated/regressive timestamps; NaN/zero scale; independent hands;
UNKNOWN identity and duplicate handedness. Pipeline coverage verifies exact
event type, hand, frame and timestamp plus observable buffer overflow.

Action coverage includes up/down signs, inversion, configured amount, replay
deduplication, preferred-hand selection, fallback, simultaneous-hand
suppression, horizontal no-op, held-button suppression, master-input disable,
swipe-scroll disable, unsupported backend and backend failure propagation.
Settings coverage includes schema-v2 sanitization, exact v2 round trip, explicit
v1-to-v2 migration and modal suspend/Cancel/Save/release-failure behavior.
The dynamic, pipeline, action, runtime-config and serialization targets were
each repeated 20 times: **100/100 executions PASS**.

## Linux desktop

A separate WSL build with `BUILD_APP=ON`, `BUILD_TESTING=OFF` and
`ENABLE_MEDIAPIPE=OFF` found Qt and OpenCV 5.0.0 and linked `c0ntrol`: **PASS**.
This compiled the recognizer, pipeline integration, dispatcher, schema codec,
QSettings adapter and the three minimal swipe-scroll controls.

## Windows CI

The first implementation run, 32788236881, reached MSVC compilation and exposed
Win32 `min/max` macro expansion in the wheel range check. It failed before
CTest; the exact C2589/C2059 errors were corrected with the macro-resistant
`(std::numeric_limits<long>::min/max)()` form.

Final Windows workflow run 32788434408 (run number 11), job 97625036607,
validated implementation head
`538fda008abce23af014008286a27ec2663d343b` with CMake 4.4.2, MSVC
19.51.36256.0 and MSBuild 18.9.1+a81b43525:

- final Windows configure/build/test: **PASS**;
- Windows CTest: **20/20 PASS** in 0.48 seconds;
- all previous Phase 7 Windows targets: **20/20 PASS**;
- `test_windows_input_backend_compile`: **PASS**;
- `WindowsSystemInputBackend::scrollVertical`: **COMPILE/LINK PASS**;
- `c0ntrol_native_input_smoke.exe` compile/link: **PASS**;
- native smoke execution: **NOT RUN**.

No workflow launches the smoke executable or emits native wheel input.
