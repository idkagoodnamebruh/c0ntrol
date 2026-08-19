# Phase 4 risk update

## Resolved

- R-010 — RESOLVED. Finger state no longer uses global `tip.y < pip.y`.
  Relative joint-angle tests preserve POINTING at 0, 45 and 90 degrees.
- R-011 — RESOLVED. Pinch uses `distance(4,8) / handScale`; tests preserve the
  ratio and static result at scales 0.5, 1.0 and 2.0.
- R-012 — RESOLVED. Explicit candidate/stable states, hysteresis, microsecond
  holds and loss cancellation produce one-shot semantic transitions.

## Open

- R-013 — OPEN. Native OS input/action dispatch belongs to Phase 5 and was not
  started. Phase 4 events are semantic only.
- R-025 — OPEN. Capture remains the existing blocking/video path; LIVE_STREAM
  and capture redesign are outside Phase 4.

## Preserved evidence

MediaPipe, its model and bridge, OneEuroFilter, and LandmarkFilterBank were not
modified. All seven previous core tests pass. Camera tuning remains pending
because this environment exposes no `/dev/video*` device; that does not weaken
the deterministic scale, rotation and temporal tests used to close these risks.
