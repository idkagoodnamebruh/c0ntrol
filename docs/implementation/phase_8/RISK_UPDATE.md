# Phase 8 risk update

## R-014 — RESOLVED

The unused frame-count tracker was removed. Timestamped palm motion now uses
hand-scale-normalized distance, real velocity, directional dominance,
OPEN_HAND gating, per-hand state, gap/loss reset and one-shot cooldown inside
the runtime `GesturePipeline`. Vertical events cross the action/backend boundary
and all required deterministic cases pass.

The initial thresholds remain hardware-tuning candidates; that does not reopen
the architectural risk addressed by R-014.

## R-022 — RESOLVED

Runtime settings are schema 2. Schema 1 preserves all Phase 7 values while only
new dynamic/scroll fields take defaults, then persists as v2. Settings and
calibration suspend native input before becoming modal, with tested Cancel and
Save semantics.

## Remaining partial risks

- R-013: **PARTIAL**. Windows wheel code compiles in CI, but no physical
  SendInput MOVE/DOWN/UP/WHEEL smoke was executed and Linux native input remains
  unsupported.
- R-015: **PARTIAL**. No physical mixed-DPI/multimonitor calibration evidence.
- R-021: **PARTIAL**. No physical-camera performance profile.
- R-025: **PARTIAL**. No physical camera loss/reconnect/shutdown validation.

MediaPipe and its bridge/model, OneEuro and LandmarkFilterBank mathematics,
static GestureEngine classification, pinch FSM semantics, AsyncCapture policy
and PointerMapper mathematics were not modified. Phase 9 has not started.
