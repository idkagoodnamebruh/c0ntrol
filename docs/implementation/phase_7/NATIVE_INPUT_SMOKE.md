# Native Windows input smoke tool

## Build and execution policy

`c0ntrol_native_input_smoke` exists only on WIN32 when
`ENABLE_NATIVE_INPUT_SMOKE=ON`; the option defaults OFF. It is an executable
target, never an `add_test`, and GitHub Actions compiles but does not run it.

Windows CI run `32705070445` built and linked
`c0ntrol_native_input_smoke.exe` with MSVC and `user32`. This is compile
evidence only. No pointer or button event was emitted by CI.

## Controlled manual sequence

When an operator launches the executable manually it:

1. initializes `WindowsSystemInputBackend` and prints virtual desktop geometry;
2. enumerates monitor rectangles read-only with `EnumDisplayMonitors`;
3. creates its own topmost test window;
4. displays an explicit OK/Cancel warning (Cancel is the default);
5. records the original cursor position;
6. moves inside the client region of its own window;
7. sends primary DOWN, a small in-window drag, and primary UP;
8. restores the original cursor and shuts the backend down.

Output reports backend initialization, virtual desktop/monitors, window
creation, consent, original cursor capture, move, DOWN, drag move, UP and
restore separately.

## Failsafe and limitations

A scope guard tracks whether DOWN succeeded. Every early return after DOWN
retries UP, restores the original cursor with `SetCursorPos`, shuts down the
backend and destroys the owned window. A normal path also verifies UP and
restore before disarming the guard.

The tool never elevates privileges or requests administrator rights. A
`SendInput` failure reports the backend's inserted-event count and
`GetLastError`; Windows does not reliably distinguish UIPI blocking through
that return path. Interactive execution on real Windows hardware remains
required before claiming native SendInput validation.
