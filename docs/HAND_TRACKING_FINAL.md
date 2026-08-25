# Hand tracking implementation status

The current hand-tracking architecture is complete through Phase 9B. This
document replaces the obsolete early closure claim that described five tests,
loose ONNX/TFLite assets and legacy gesture classes.

## Runtime architecture

```text
AsyncCapture
  -> MediaPipeHandTrackingBackend or MockHandTrackingBackend
  -> LandmarkFilterBank
  -> HandFeatureExtractor
  -> GestureEngine
  -> GestureStateMachine
  -> DynamicGestureRecognizer
  -> ActionDispatcher
  -> Windows SendInput / Linux Wayland EIS / Null backend
```

The static poses implemented by the modern engine are `POINTING`, `PINCH` and
`OPEN_HAND`. The timestamped dynamic recognizer emits one-shot swipe events in
four directions; vertical events can dispatch configurable scrolling.

## Model contract

`models/hand_landmarker.task` is the only required inference asset. The model
download script uses an official versioned MediaPipe URL, validates size and
SHA-256, and installs atomically. It never converts a download error into an
empty placeholder. Exact source, identity and consumer evidence are recorded
in `docs/implementation/phase_9/MODEL_ASSET_AUDIT.md`.

## Build truth

The ordinary build leaves `ENABLE_MEDIAPIPE=OFF` and therefore uses the mock
backend. Real inference requires both `ENABLE_MEDIAPIPE=ON` and the bridge
library built from the project-pinned MediaPipe v0.10.26 source. Linux native
input additionally requires libei/liboeffis 1.2.1 or newer; otherwise the Null
backend is selected unless strict dependency mode was requested.

## Validation boundaries

Automated tests cover tracking contracts, filtering, gesture geometry/FSM,
dynamic recognition, pointer/action mapping, configuration, asynchronous
capture, platform adapters and model tooling. Real static-image MediaPipe
inference passed in Phase 2. Windows and Linux platform builds are exercised in
GitHub Actions, but physical camera, SendInput and Wayland portal validation
remain explicitly tracked as partial risks. Automated jobs do not emit native
input.

The live GUI currently instantiates camera output, the developer telemetry
widget and Settings. Minimalist and Matrix widget sources remain disconnected;
they are not active product modes.
