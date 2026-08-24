# RuntimeConfig contract

`RuntimeConfig` is Qt-free and has schema version 1. It owns the canonical
runtime configuration graph:

```text
RuntimeConfig
  |-- CameraConfig
  |-- PointerMappingConfig
  |-- LandmarkFilterConfig
  |     |-- normalized OneEuroConfig
  |     `-- world OneEuroConfig
  |-- GestureConfig
  `-- InputConfig
        |-- enabled (default false)
        `-- preferredHand (default RIGHT)
```

Default member initializers are the single default source. Sanitizers obtain
fallbacks by default-constructing the corresponding config rather than defining
another runtime profile.

## Validation

`sanitizeRuntimeConfig()` normalizes the schema and delegates to component
sanitizers. It enforces:

- camera index >= 0, dimensions 16..16384, finite FPS in (0, 1000], and buffer
  size 1..64;
- finite pointer margins in [0, 1), with opposing sums below 1;
- positive OneEuro cutoffs/max delta, beta >= 0, positive filter reset timeout
  and teleport threshold;
- finite positive pinch thresholds with enter < exit, nonnegative hold/loss
  durations, and valid finite gesture geometry thresholds;
- preferred hand LEFT or RIGHT.

NaN and infinity are never accepted. Invalid fields return to their canonical
safe defaults. Native input defaults to disabled, so a new installation can
open, capture and classify without moving or clicking the system pointer.

## Injection and application

- Main startup loads/sanitizes settings before constructing MainWindow.
- `VisionWorker` receives `CameraConfig` and `LandmarkFilterConfig` explicitly.
- `OpenCVCameraSource`, and therefore its `AsyncCapture`, receives the loaded
  camera request.
- `GesturePipeline` receives loaded `GestureConfig`.
- `ActionDispatcher` receives loaded pointer and input configs.
- `RuntimeConfigController` computes changes and owns safe release ordering. It
  does not own Qt, storage, capture or the native backend.

Hot-applied values are pointer mirror/margins, input enabled/preferred hand,
filter config and gesture config. Pointer/input changes first release any held
button. Gesture replacement resets its temporal state, and filter replacement
resets filter state on the vision thread.

Any camera index/resolution/FPS/buffer change requires a controlled capture
stop/configure/start. `RuntimeConfigController` releases input before MainWindow
invokes that restart synchronously on the vision thread.
