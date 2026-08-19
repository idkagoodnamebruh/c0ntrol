# Phase 4 handoff

## Approved base and scope

Phase 4 starts from exact Phase 3 head
`8d6fca4810a41854fbe100949717d2d2b6964481` and introduces only the gesture
feature/recognition/temporal pipeline plus its runtime adapter, tests and docs.

## Stable contracts for the next phase

- Consume `GesturePipelineResult.events`; do not reconstruct pinch from static
  pose labels or raw landmarks.
- `POINTER_ACTIVE` and `POINTER_INACTIVE` are edges, not per-frame movement.
- `PINCH_BEGIN`, `PINCH_END` and `PINCH_CANCEL` are one-shot semantic edges.
- Event pointer coordinates remain normalized camera coordinates. Any display,
  multi-monitor, acceleration or calibration transform belongs downstream.
- LEFT and RIGHT FSM state is independent. UNKNOWN/duplicate handedness is
  intentionally excluded from nominal state.
- Invalid or missing tracking cancels a confirmed pinch once after 150,000 us.
- Defaults 0.25/0.35 and 75 ms are initial tested values, not camera-tuned
  universal constants.

## Verification before downstream work

Run `make test`; the expected Phase 4 result is 10/10 PASS. Desktop compilation
with MediaPipe disabled is known to pass. Phase 2 MediaPipe validation remains
the authoritative real-inference evidence because the backend was unchanged
and its bridge artifact was unavailable here.

R-010, R-011 and R-012 are resolved. R-013 and R-025 remain open. Native OS
input, action dispatch, persistent configuration and Phase 5 were not started.
