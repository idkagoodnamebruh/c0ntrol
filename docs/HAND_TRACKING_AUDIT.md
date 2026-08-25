# Hand tracking audit status

This file originally described the Phase 0 mock-only baseline. Its classifier,
cursor and loose-model recommendations were superseded by the audited Phase 2
through Phase 9 implementation and must not be read as the current runtime.

## Current tracking path

The desktop selects `MediaPipeHandTrackingBackend` only when built with
`ENABLE_MEDIAPIPE=ON` and an explicit `MEDIAPIPE_BRIDGE_LIBRARY`; otherwise it
selects `MockHandTrackingBackend`. The real backend consumes the sole canonical
asset `models/hand_landmarker.task`. Phase 2 validated model creation, video
inference, one detected hand, 21 normalized landmarks, 21 world landmarks and
handedness through the C ABI bridge.

After tracking, `LandmarkFilterBank` maintains independent One Euro state per
hand and coordinate. `HandFeatureExtractor`, `GestureEngine`,
`GestureStateMachine` and `DynamicGestureRecognizer` produce current static and
temporal observations. `ActionDispatcher` maps those observations through
Windows SendInput, Linux Wayland/EIS when available, or an explicit Null
backend.

Current authoritative evidence:

- `docs/implementation/phase_2/REAL_INFERENCE_VALIDATION.md`;
- `docs/implementation/phase_9/MODEL_ASSET_AUDIT.md`;
- `docs/implementation/phase_9/TEST_REPORT.md`.

Phase 9B removed the unconsumed loose detector assets and the zero-byte ONNX
placeholder. The historical Phase 0 asset inventory remains intentionally
unchanged as a dated baseline.
