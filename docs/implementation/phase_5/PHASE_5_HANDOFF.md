# Phase 5 handoff

## Stable boundary

Downstream work must consume `ActionDispatcher`/`ISystemInputBackend`; it must
not reintroduce action decisions into GestureEngine, GestureStateMachine or
MainWindow. `ActionCommand` and `DesktopGeometry` are Qt/Win32-free.

The current controller policy is temporary: pointer-active RIGHT, otherwise
pointer-active LEFT. Switching always releases a button owned by the old hand.
Do not transfer a held button between nominal handedness slots.

PointerMapper defaults to no mirror and zero margins. Preserve the single
camera-normalized -> virtual-pixel -> SendInput-absolute path. Any calibration
or persistent settings must configure this path, not add another mapper.

## Required follow-up before claiming Windows support

The backend now compiles with MSVC, links with `user32` and passes the 14-test
Windows headless suite. Remaining native follow-up is:

1. Run an explicit, manual/opt-in test in a controlled app-owned window.
2. Verify virtual desktop edges, monitors with negative origins and mixed DPI.
3. Verify SendInput failure reporting against higher-integrity/UIPI targets.
4. Restore the original cursor position and guarantee UP during every test.

Do not put native input into normal CTest. R-013 is PARTIALLY_RESOLVED and R-015
remains PARTIAL until native validation exists. R-023 is resolved.
Linux/Wayland input and Phase 6 were not started.
