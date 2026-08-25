# Linux EIS system input backend

## Architecture

`LinuxEisSystemInputBackend` implements the existing
`ISystemInputBackend` contract. `IEisInputSession` is the narrow transport seam:
production uses `LibeiPortalSession`, while unit tests inject a deterministic
fake. Core headers contain no libei, liboeffis, D-Bus or Qt types.

Factory policy is now:

- `_WIN32` -> `WindowsSystemInputBackend`;
- Linux build with `C0NTROL_HAS_LIBEI` -> `LinuxEisSystemInputBackend`;
- missing optional dependencies or other platforms ->
  `NullSystemInputBackend`.

The real-library compile target defines `C0NTROL_HAS_LIBEI`, calls the factory,
and asserts the selected dynamic type without calling `initialize()`. This
proves headers, libraries, factory wiring and destruction without opening the
portal.

## Contract mapping

- `desktopGeometry`: overflow-safe bounding union of active EIS regions;
- `movePointer`: absolute only, with aggregate and per-device region checks;
- `primaryButtonDown/Up`: `BTN_LEFT`, duplicate DOWN suppression and defensive
  owned-state tracking;
- `scrollVertical`: signed logical notches converted to libei discrete units;
- `shutdown`: best-effort UP, stop emulation, release all contexts;
- `lastError`: portal, EIS, capability, region and operation failures.

No relative-motion fallback exists because the upstream action contract
provides absolute desktop coordinates. No operation requires root,
`/dev/uinput`, XTest, xdotool or X11.
