# Windows build validation

## GitHub Actions evidence

- Workflow: `Phase 5 Windows Build`.
- Run: `32558529713` (run number 2).
- Validated commit: `94d61a16b4acbc5234df03a8bcaadcb368493aee`.
- Runner label: `windows-latest`.
- Observed runner OS: Microsoft Windows Server 2025, build 10.0.26100.
- CMake: 4.4.2.
- Generator: Visual Studio 18 2026.
- Compiler: MSVC 19.51.36256.0.
- MSBuild: 18.9.1+a81b43525.

The Release build compiled `WindowsSystemInputBackend.cpp`,
`WindowsPointerMath.cpp` and `test_windows_input_backend_compile.cpp`. The
resulting target linked successfully with `user32` and produced
`test_windows_input_backend_compile.exe`.

## Results

- Windows backend compile: PASS.
- `user32` link: PASS.
- WindowsPointerMath: PASS.
- Windows CTest: 14/14 PASS.
- Native SendInput execution: NOT RUN.
- Linux core regression: 13/13 PASS.
- Linux desktop build with MediaPipe disabled: PASS.

The compile-only test constructs the backend, reads `lastError()` and calls
`shutdown()`. It never calls `initialize()`, `movePointer()`,
`primaryButtonDown()` or `primaryButtonUp()`, so CI cannot move the pointer or
emit a native button transition.

This validates the Windows SDK surface and link boundary, not interactive
injection behavior, UIPI handling or physical mixed-DPI/multimonitor behavior.

