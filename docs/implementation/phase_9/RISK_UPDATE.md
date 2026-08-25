# Phase 9 risk update

## R-007 — RESOLVED

Phase 9B removed every downloader conflict marker, all silent critical-error
suppression and placeholder creation. The canonical model is now checked by
minimum size and SHA-256; downloads are temporary, validated and atomically
installed; the offline failure path returns non-zero and leaves no final file.
The local tooling test passes, and published Linux run `32814406645` repeated
the same checks before the strict build. There are no tracked zero-byte model
files. R-007 is **RESOLVED**.

## R-013 — PARTIAL

Windows native input remains compiled and regression-tested. Linux
Wayland/EIS is now implemented, selected by the factory, compiled and linked
against Ubuntu 24.04 libei/liboeffis 1.2.1, and tested through a fake session.

R-013 remains partial because neither physical Windows SendInput nor a physical
Wayland RemoteDesktop/EIS session was run. X11 is intentionally not
implemented.

## Other tracked risks

- R-006: **RESOLVED**. The canonical task bundle remains the exact model proven
  by real MediaPipe creation and inference in Phase 2.
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
- R-026: **OPEN**. Minimalist and Matrix widget sources remain compiled but are
  not instantiated or advertised as active runtime modes.
- R-027: **OPEN**. Packaging and installation remain future work.

MediaPipe, OneEuro and LandmarkFilterBank mathematics, static GestureEngine,
pinch FSM, dynamic thresholds, AsyncCapture and PointerMapper mathematics were
not modified. Phase 10 has not started.
