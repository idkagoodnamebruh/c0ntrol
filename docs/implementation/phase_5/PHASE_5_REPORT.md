# Phase 5 report

## Status

Phase 5 is complete at the architecture and headless-validation level on the
exact approved Phase 4 head `a437707ccc981e1059cad4c08537f0cdec67eefd`.

The runtime path is now:

`GesturePipeline -> ActionDispatcher -> PointerMapper -> ISystemInputBackend`.

On Windows, the platform factory selects `WindowsSystemInputBackend`, which
uses `SendInput`. On unsupported platforms it selects an explicit null backend
that fails initialization rather than pretending success. Linux native input
was not started.

## Delivered behavior

- RIGHT is the temporary controlling hand when it is pointer-active; otherwise
  LEFT may control. Both hands never move the cursor simultaneously.
- An active observation produces one MOVE per new valid timestamp.
- PINCH_BEGIN produces one primary DOWN; repeated BEGIN is suppressed.
- PINCH_END and PINCH_CANCEL produce one primary UP when c0ntrol owns DOWN.
- Switching hands, disabling input, tracking loss, mapping/backend failure and
  shutdown release a held button without transferring ownership.
- A successful DOWN followed by a failed MOVE gets one best-effort recovery UP.
- Duplicate/regressive timestamps produce no backend calls.
- Pointer movement remains active while pinched, yielding downstream drag
  semantics without adding a DRAG gesture.

`MainWindow` only invokes the dispatcher and presents telemetry/errors. The
Qt `CursorController/QCursor` path and the old `DisplayTransform` mapping were
removed, leaving one active pointer transformation boundary.

## Validation

- Clean core CTest: 13/13 PASS.
- Previous Phase 4 targets: 10/10 PASS.
- New action/mapping targets: 3/3 PASS.
- `make test`: 13/13 PASS.
- Linux/WSL desktop build with MediaPipe disabled: PASS.
- Windows pointer/absolute-coordinate math: PASS headless.
- Windows backend source: IMPLEMENTED with `SendInput`.
- Windows backend compilation: NOT AVAILABLE (no MSVC, Windows SDK, MinGW or
  cross-toolchain present).
- Native Windows integration: NOT TESTED; no automatic test moved the user's
  real cursor or button.

MediaPipe, OneEuroFilter, LandmarkFilterBank, HandFeatureExtractor,
GestureEngine and GestureStateMachine were not modified functionally. Phase 6
was not started.
