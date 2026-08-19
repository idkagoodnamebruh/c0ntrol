# Gesture state machine

There is one `GestureStateMachine` for each nominal LEFT/RIGHT hand.

```text
IDLE
  -- ratio <= 0.25 --> CANDIDATE_DOWN
CANDIDATE_DOWN
  -- ratio > 0.25 ----------------------> IDLE
  -- held 75,000 us --------------------> PINCHED + PINCH_BEGIN
PINCHED
  -- ratio >= 0.35 ---------------------> CANDIDATE_UP
CANDIDATE_UP
  -- ratio < 0.35 ----------------------> PINCHED
  -- held 75,000 us --------------------> IDLE + PINCH_END
```

The gap from 0.25 to 0.35 is hysteresis. One low sample cannot begin a pinch,
and values inside the gap cannot release a confirmed pinch. Hold duration uses
microsecond timestamps, not frame counts. A confirmed pinch emits exactly one
BEGIN and one END per complete cycle; no per-frame HOLD is emitted.

Pointer activity is an independent edge state and emits `POINTER_ACTIVE` or
`POINTER_INACTIVE` only on transitions.

## Missing and invalid tracking

An invalid observation, absent named hand, UNKNOWN hand or duplicate nominal
hand is treated as missing for that FSM. Pointer activity ends immediately.
An entry candidate is discarded; a release candidate returns to PINCHED. A
confirmed pinch may recover within 150,000 us without a false END. If loss
reaches 150,000 us, the FSM emits one `PINCH_CANCEL` and returns to IDLE.

Repeated, regressive or negative timestamps are ignored without state mutation
or events. This prevents time anomalies from satisfying debounce accidentally.

`reset()` clears all temporal, pointer and timestamp state.
