# Phase 4 report

## Status

Phase 4 is complete on top of approved Phase 3 commit
`8d6fca4810a41854fbe100949717d2d2b6964481`.

The active path is now:

`HandTrackingFrame (filtered) -> HandFeatureExtractor -> GestureEngine -> GestureStateMachine -> GestureEvent`.

The implementation adds normalized continuous features, orientation-independent
joint geometry, independent LEFT/RIGHT state machines, pinch hysteresis and
timestamp-based debounce. It produces semantic transition events only. No
native click, key, scroll, drag, or platform input backend was added.

## Delivered contracts

- `HandFeatures` contains validity, hand scale, palm center, pointer point,
  normalized pinch ratio, five curl values, extension flags and handedness.
- `GestureObservation` keeps static pose, pointer and pinch signals orthogonal,
  so `POINTING + PINCH` can coexist.
- `GestureEvent` carries type, nominal handedness, frame id, timestamp and
  normalized pointer point.
- `GesturePipeline` owns one FSM per nominal LEFT/RIGHT slot. UNKNOWN and
  duplicate handedness never reuse a named slot ambiguously.
- The old `GestureClassifier` was removed from runtime and source. The GUI now
  observes core results and does not calculate angles, ratios or debounce.
- The legacy cursor movement adapter may still consume `pointerPoint`, but the
  old per-frame `PINCH -> performClick()` wiring is gone. Pinch transitions are
  not connected to OS actions.

## Defaults

| Setting | Value |
|---|---:|
| pinch enter ratio | 0.25 |
| pinch exit ratio | 0.35 |
| enter hold | 75,000 us |
| exit hold | 75,000 us |
| tracking-loss timeout | 150,000 us |
| extended max curl | 0.22 |
| curled min curl | 0.38 |

These are deterministic initial defaults validated with synthetic geometry.
They remain candidates for camera tuning when a camera is available.

## Validation result

- Clean core configure/build/CTest: 10/10 PASS.
- Previous Phase 3 tests: 7/7 PASS.
- New gesture tests: 3/3 PASS.
- `make test`: 10/10 PASS.
- Desktop build, `BUILD_APP=ON`, `ENABLE_MEDIAPIPE=OFF`: PASS.
- Camera: NOT AVAILABLE (`/dev/video*` absent).
- MediaPipe bridge/backend/model: NOT MODIFIED; the prior validated bridge was
  not present in this environment, so MediaPipe inference was not repeated.
- OneEuroFilter and LandmarkFilterBank: NOT MODIFIED.

Phase 5 was not started.
