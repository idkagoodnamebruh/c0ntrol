# Linux input baseline

## Approved base

Phase 9 starts at the published Phase 8 head
`05d0060a2b2aeb31c8da29c48376497d1941ad30`.

Before this phase, `SystemInputBackendFactory` selected
`WindowsSystemInputBackend` under `_WIN32` and
`NullSystemInputBackend` everywhere else. Linux therefore exposed explicit
unsupported errors for move, button and scroll.

## Previous dispatcher lifecycle

- `ActionDispatcher::initialize()` immediately initialized the platform
  backend and required valid desktop geometry, even when `InputConfig.enabled`
  was false.
- `setInputEnabled()` only toggled an already initialized backend.
- `applyConfiguration()` refused to enable an uninitialized backend.
- `shutdown()` released the primary button, shut down the backend and was
  idempotent at dispatcher level.

That eager lifecycle was safe for non-interactive Win32 initialization but
would have opened a RemoteDesktop consent dialog on first run. Phase 9 keeps
the public action contract and moves native initialization behind explicit
enablement.

## Preserved boundaries

The Linux implementation is below `ISystemInputBackend`. It does not change
MediaPipe, filtering mathematics, static gesture classification, pinch FSM,
AsyncCapture or PointerMapper mathematics. It does not use X11, XTest,
`/dev/uinput`, root privileges or shell commands for input.
