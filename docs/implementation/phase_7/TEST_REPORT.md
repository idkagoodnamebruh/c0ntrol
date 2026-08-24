# Phase 7 test report

## Linux core

Clean WSL configuration used GCC 16.2.1 and CMake 4.4.2:

```text
cmake -S . -B /tmp/c0ntrol-phase7-core \
  -DBUILD_APP=OFF -DBUILD_TESTING=ON -DENABLE_MEDIAPIPE=OFF
cmake --build /tmp/c0ntrol-phase7-core --parallel
ctest --test-dir /tmp/c0ntrol-phase7-core --output-on-failure
```

Result: **19/19 PASS**. All previous Phase 6 Linux targets remain **16/16
PASS**. New targets are:

- `test_runtime_config`: PASS;
- `test_settings_serialization`: PASS;
- `test_pointer_calibration`: PASS.

`test_action_dispatcher` was expanded for disabled-first-run, explicit enable,
preferred hand, disable/restart/reset releases and remains PASS. Those four
settings/calibration/safety targets were each repeated 20 times: **80/80
executions PASS**.

Settings coverage includes defaults, full round trip, missing values, corrupt
strings, range violations, NaN, infinity, schema 0 migration, future schema,
unknown keys and persisted reset. Calibration covers valid/mirrored regions,
multiple samples, a strong outlier, reversed/degenerate range, non-finite input
and the resulting unique PointerMapper output.

## Linux desktop

Clean `BUILD_APP=ON`, `BUILD_TESTING=OFF`, `ENABLE_MEDIAPIPE=OFF` found Qt and
OpenCV 5.0.0, compiled QSettings, both dialogs, RuntimeConfig integration and
VisionWorker camera reconfiguration, and linked `c0ntrol`: **PASS**.

## Windows CI

- Workflow run: `32705070445` (run number 8).
- Job: `97364285875` on `windows-latest`.
- Validated implementation commit:
  `4a63f44a21d3fa0b8246bca9b50d462eff506d45`.
- CMake 4.4.2, MSVC 19.51.36256.0, MSBuild 18.9.1+a81b43525.
- Configure/build/test: PASS.
- Windows CTest: **20/20 PASS** in 0.48 seconds.
- Previous Windows tests: **17/17 PASS**.
- New settings/calibration tests: **3/3 PASS**.
- `test_windows_input_backend_compile`: PASS.
- `c0ntrol_native_input_smoke.exe`: COMPILE/LINK PASS.
- Native smoke execution: NOT RUN.

The smoke executable is not among the 20 CTest targets. No GitHub Actions step
launches it.
