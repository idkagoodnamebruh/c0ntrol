# Settings baseline

## Approved Phase 6 tree

The audit used approved Phase 6 head
`742e8a6e8e9c1b6dc1875ac8eef4bcfaf17dd67d`.

The only settings-shaped type was `src/core/config/Settings.h`:
`AppSettings` contained camera index/FPS/size, obsolete gesture thresholds and
an ONNX model path. No source constructed it, loaded it, saved it or injected it
into runtime components. It was dead code and its model path did not represent
the Phase 2 MediaPipe task model.

There was no `QSettings`, settings-store boundary, schema version, migration,
corruption handling or reset-to-default flow.

## Values actually consumed before Phase 7

- `VisionWorker` defaulted camera index to 0. `CameraConfig` lived in the OpenCV
  adapter and independently defaulted to 640 x 480, 30 requested FPS and buffer
  size 1.
- `PointerMappingConfig` defaulted mirror flags and four margins in
  `PointerMapper`; MainWindow always constructed the mapper with those defaults.
- `LandmarkFilterConfig` and both `OneEuroConfig` channels were default-
  constructed inside VisionWorker. Only the filter-enabled flag had a setter,
  and no UI persisted it.
- `GestureConfig` was default-constructed inside MainWindow's
  `GesturePipeline`. Gesture subcomponents sanitized their own copies.
- `ActionDispatcher` hardcoded RIGHT-hand preference and started with native
  input enabled after backend initialization. There was no persisted consent.
- MainWindow immediately constructed camera, gesture and native-input objects;
  no settings load preceded runtime construction.

## Hardcoded/default duplication

Camera defaults existed separately from dead `AppSettings`. Gesture defaults
also appeared as literal sanitizer fallbacks. Landmark-filter and One-Euro
fallbacks repeated values from their default structs. Pointer invalid-value
fallbacks used separate zero literals.

Phase 7 removes dead `AppSettings`, makes each config struct the canonical
default source, and routes all persisted values through one Qt-free
`RuntimeConfig` sanitizer before construction or hot application.
