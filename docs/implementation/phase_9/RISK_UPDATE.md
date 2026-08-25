# Phase 9 risk update

## R-013 — PARTIAL

Windows native input remains compiled and regression-tested. Linux
Wayland/EIS is now implemented, selected by the factory, compiled and linked
against Ubuntu 24.04 libei/liboeffis 1.2.1, and tested through a fake session.

R-013 remains partial because neither physical Windows SendInput nor a physical
Wayland RemoteDesktop/EIS session was run. X11 is intentionally not
implemented.

## Other tracked risks

- R-014: **RESOLVED**. The timestamped dynamic recognizer remains in place; the
  inherited pose-flicker test now proves OPEN_HAND gating cannot cancel an
  active cooldown.
- R-015: **PARTIAL**. EIS multi-region unions and monitor-gap rejection are
  deterministic, but no physical mixed-DPI/multimonitor calibration was run.
- R-021: **PARTIAL**. No physical-camera performance profile was run.
- R-022: **RESOLVED**. Schema v2 and modal input suspension are preserved; a
  failed portal activation cannot be committed as enabled runtime state.
- R-025: **PARTIAL**. Portal/EIS disconnect and device pause/remove are tested,
  but physical camera/session reconnect and shutdown remain unvalidated.

MediaPipe, OneEuro and LandmarkFilterBank mathematics, static GestureEngine,
pinch FSM, dynamic thresholds, AsyncCapture and PointerMapper mathematics were
not modified. Phase 10 has not started.
