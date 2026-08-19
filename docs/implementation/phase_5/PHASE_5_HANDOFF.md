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

1. Compile the Windows backend with a real Windows SDK/toolchain.
2. Run an explicit, manual/opt-in test in a controlled app-owned window.
3. Verify virtual desktop edges, monitors with negative origins and mixed DPI.
4. Verify SendInput failure reporting against higher-integrity/UIPI targets.
5. Restore the original cursor position and guarantee UP during every test.

Do not put native input into normal CTest. R-013 stays OPEN and R-015 PARTIAL
until this validation exists. R-023 is resolved. Linux/Wayland input and Phase 6
were not started.
