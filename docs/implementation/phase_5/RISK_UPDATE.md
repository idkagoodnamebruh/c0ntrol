# Phase 5 risk update

- R-008 — RESOLVED (Phase 3), unchanged.
- R-009 — RESOLVED (Phase 3), unchanged.
- R-010 — RESOLVED (Phase 4), GestureEngine unchanged.
- R-011 — RESOLVED (Phase 4), normalized pinch unchanged.
- R-012 — RESOLVED (Phase 4), GestureStateMachine unchanged.
- R-013 — PARTIALLY_RESOLVED. The Windows `SendInput` implementation now
  compiles with MSVC and links with `user32`, and all 14 Windows headless tests
  pass. Native SendInput execution remains unvalidated and a Linux native input
  backend is intentionally absent.
- R-015 — PARTIAL. Virtual-desktop origins/dimensions, mirrors, active regions
  and negative-origin math are implemented/tested. Real mixed-DPI/multimonitor
  Windows behavior remains unvalidated.
- R-023 — RESOLVED. `CursorController/QCursor` and legacy `DisplayTransform`
  mapping were removed; PointerMapper is the only active transformation.
- R-025 — OPEN. Capture scheduling/LIVE_STREAM is outside Phase 5.

No risk state relies on the unsupported null backend pretending success.
