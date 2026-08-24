# Phase 7 risk update

## R-022 — RESOLVED

Dead `AppSettings` was removed. Versioned settings are now loaded before runtime
construction, sanitized, migrated, persisted and explicitly injected into
camera, filters, gestures, pointer mapping and input policy. Qt storage remains
outside core and reset/default/error behavior is covered by tests.

## R-015 — PARTIAL

Pointer active-region calibration now uses robust multi-sample medians and feeds
the existing mapper. Mathematical tests cover margins, mirrors, negative
virtual origins and calibrated edges. The manual Windows tool can report real
virtual-desktop and monitor rectangles.

It was not executed on physical Windows hardware, so real mixed-DPI and
multimonitor mapping evidence does not exist. R-015 remains PARTIAL.

## R-013 — PARTIAL

The Windows backend compile/link regression remains green and the controlled,
failsafe, opt-in smoke executable now compiles on Windows. The executable was
not run; MOVE/DOWN/UP/restore and UIPI behavior remain unobserved. R-013 is not
marked Windows-resolved. Linux native input is still intentionally absent.

## Unchanged risks

- R-021 remains PARTIAL pending profiling with a physical camera and MediaPipe.
- R-025 remains PARTIAL pending real camera read/disconnect/shutdown evidence.

MediaPipe, AsyncCapture latest-frame policy, gesture classification/FSM logic
and ActionDispatcher button transition semantics were not functionally
modified. Phase 8 has not started.
