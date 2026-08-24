# Phase 7 handoff

## Stable startup contract

```text
QtSettingsStore load
  -> schema decode / sanitize / migrate
  -> RuntimeConfig
  -> construct camera/filter/gesture/pointer/input components
  -> start capture and processing
```

Do not move QSettings into core components or add a mutable settings singleton.
New persisted fields belong in the canonical config struct, schema codec,
sanitizer and both round-trip tests. Increment `configVersion` only with an
explicit migration policy.

Native input must remain disabled in `InputConfig{}`. Backend initialization may
read desktop geometry, but no movement/button command may occur until the user
persists enablement.

## Stable runtime-change contract

- Pointer/input: release held input, replace the sole mapper/preference, reset
  ownership/timestamp state, then continue.
- Camera: release input, stop/join capture, set camera request, restart capture.
- Filters: replace/reset on the vision thread.
- Gestures: replace/reset GesturePipeline on the GUI thread.
- Reset: force input release, apply complete canonical defaults and persist.

If release fails, `RuntimeConfigController` rejects the change and retains the
previous config. Do not bypass it when adding settings UI.

## Calibration contract

Keep calibration independent of OS input and use its optional result. Invalid,
undersampled or degenerate samples must never erase the prior mapping. The
calibrated margins continue through `PointerMapper`; no second display mapping
is allowed.

## Remaining manual evidence

On a controlled physical Windows machine, explicitly build/run
`c0ntrol_native_input_smoke`, accept its warning, and record every reported
operation plus monitor topology/DPI context. Confirm the pointer is restored and
LEFT UP occurs even after an induced post-DOWN failure. Until that happens:

- R-013: PARTIAL;
- R-015: PARTIAL.

Physical-camera profiling/shutdown is still required for R-021/R-025. Native
Linux input, new gestures and Phase 8 were not started.
