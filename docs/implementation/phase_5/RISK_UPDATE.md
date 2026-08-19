# Phase 5 risk update

- R-008 — RESOLVED (Phase 3), unchanged.
- R-009 — RESOLVED (Phase 3), unchanged.
- R-010 — RESOLVED (Phase 4), GestureEngine unchanged.
- R-011 — RESOLVED (Phase 4), normalized pinch unchanged.
- R-012 — RESOLVED (Phase 4), GestureStateMachine unchanged.
- R-013 — OPEN. The Windows `SendInput` backend is implemented, but no Windows
  compiler/session was available and Linux native input is intentionally absent.
  It cannot honestly be marked partially resolved until Windows build/native
  integration is validated.
- R-015 — PARTIAL. Virtual-desktop origins/dimensions, mirrors, active regions
  and negative-origin math are implemented/tested. Real mixed-DPI/multimonitor
  Windows behavior remains unvalidated.
- R-023 — RESOLVED. `CursorController/QCursor` and legacy `DisplayTransform`
  mapping were removed; PointerMapper is the only active transformation.
- R-025 — OPEN. Capture scheduling/LIVE_STREAM is outside Phase 5.

No risk state relies on the unsupported null backend pretending success.
