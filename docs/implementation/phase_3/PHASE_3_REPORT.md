# Phase 3 report — real-landmark stabilization

## Status

**COMPLETE** on branch `implementation/phase-3-landmark-filtering`, based exactly on approved Phase 2 remote head `6535e2248b540c175bb5b22e958cce6533360500`.

Phase 3 replaces the defective multi-purpose `OneEuroFilter` with a canonical scalar filter and adds `LandmarkFilterBank`. The backend-owned `HandTrackingFrame` remains raw and unchanged. `VisionWorker` emits that raw frame, derives and emits a separate filtered frame, and sends only the filtered frame through `toLegacyLandmarks` to the existing GUI/cursor path.

## Delivered behavior

- `OneEuroFilter` represents one scalar channel and uses seconds derived from the frame timestamp.
- `minCutoff`, `beta`, and derivative cutoff all participate in the canonical equations.
- Every unambiguous LEFT and RIGHT slot owns 63 normalized scalar filters and 63 separate world-coordinate filters.
- Hand array order is irrelevant; association is by unique handedness only.
- UNKNOWN and duplicate handedness are conservative raw-passthrough cases that reset potentially ambiguous state.
- A hand slot resets after 400,000 microseconds without an eligible observation.
- A normalized wrist displacement over 0.35 resets both coordinate banks for that hand.
- Repeated, regressive, non-finite, and excessively separated scalar timestamps cannot divide by zero or retain stale interpolation state.
- NaN/Inf samples do not update filter state.
- Filtering can be toggled at runtime through `VisionWorker::setFilteringEnabled`; a transition resets all states.

Defaults are initial tuning values only: normalized/world `minCutoff=1.0`, `beta=0.05`, `derivativeCutoff=1.0`; live-camera tuning remains future hardware work.

## Validation summary

- Core CTest suite: **7/7 PASS**.
- Existing Phase 2 test targets: **6/6 PASS**.
- Filtering targets (`test_one_euro`, `test_landmark_filter_bank`): **2/2 PASS**.
- Project `make test`: **PASS, 7/7**.
- Desktop build, MediaPipe OFF: **PASS**.
- MediaPipe tests: **NOT RE-RUN — external bridge artifact unavailable in this host**. The MediaPipe backend, model, pin, and Phase 2 results were not changed.
- Camera tuning: **NOT AVAILABLE** (`/dev/video*` absent in WSL).

R-008 and R-009 are resolved by implementation plus tests. R-010, R-011, R-012, R-013, and R-025 remain open. Gesture FSM, native input, and Phase 4 were not started.
