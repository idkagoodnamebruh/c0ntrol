# Action architecture

```text
filtered HandTrackingFrame
  -> GesturePipeline
  -> GesturePipelineResult
  -> ActionDispatcher
  -> PointerMapper
  -> ISystemInputBackend
      -> WindowsSystemInputBackend (Windows)
      -> NullSystemInputBackend (unsupported platform)
```

## Responsibilities

- `GesturePipeline` owns hand features and temporal gesture semantics. It has
  no action or OS dependency.
- `ActionDispatcher` owns the temporary controlling-hand policy, strict
  timestamp acceptance, primary-button ownership and safety releases. It
  translates observations/events to domain `ActionCommand` values.
- `PointerMapper` clamps normalized camera points, applies one mirror/active
  region policy and maps to virtual-desktop pixels.
- `ISystemInputBackend` is the Qt/gesture-independent platform boundary.
- `WindowsSystemInputBackend` alone knows `INPUT`, `MOUSEINPUT`,
  `GetSystemMetrics` and `SendInput`.
- `MainWindow` observes labels/events/errors and forwards the pipeline result;
  it does not select the action hand or move the cursor itself.

## Ownership and release

DOWN is recorded only after the backend reports success. UP is sent only when
c0ntrol owns that DOWN. An UP failure keeps internal ownership so disable or
shutdown can retry once later. A move failure after successful DOWN attempts
one immediate UP and never loops.

The temporary hand policy is RIGHT when RIGHT is valid and pointer-active,
otherwise LEFT. On any owner change, the previous DOWN is released before the
new hand can emit an action. UNKNOWN never reuses LEFT/RIGHT state. Persistent
tracking identity and ownership are deliberately deferred.

`releaseAll()` is used for controller change, disable, shutdown and failure
recovery. `ActionDispatcher::shutdown()` releases first and calls backend
shutdown second; its destructor repeats no work after explicit shutdown.
