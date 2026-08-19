# Landmark filter bank

## RAW and FILTERED observations

The core pipeline is:

```text
backend.process -> raw HandTrackingFrame
                -> trackingFrameProcessed(raw)
                -> LandmarkFilterBank::process(raw)
                -> filteredTrackingFrameProcessed(filtered)
                -> toLegacyLandmarks(filtered)
                -> existing frameProcessed / GUI / cursor path
```

`process` takes a const raw frame and returns a copy. It preserves `valid`, `timestampUs`, `frameId`, handedness, and handedness score. It changes only finite-filtered landmark coordinates. An invalid input remains invalid and has no output hands; a valid zero-hand frame remains valid and empty. No hand is fabricated.

Both Qt signals reuse the existing `HandTrackingFrame` metatype. The bank itself has no Qt dependency and runs in the worker thread.

## Persistent state layout

Each unique handedness slot owns:

- 21 × 3 = 63 normalized filters;
- 21 × 3 = 63 world filters.

LEFT and RIGHT therefore never share history, and normalized/world history never mixes. The objects are persistent `std::array` members; there is no heap allocation per axis per frame.

## Conservative association policy

- A single LEFT uses only the LEFT slot.
- A single RIGHT uses only the RIGHT slot.
- Swapping `[LEFT, RIGHT]` to `[RIGHT, LEFT]` does not swap filter states.
- Any UNKNOWN hand passes through raw and resets both named slots before eligible named hands in that frame establish fresh state.
- Duplicate LEFT or duplicate RIGHT observations pass through raw and reset only the duplicated slot.

This deliberately trades a brief raw frame for isolation. Without a persistent tracking ID, it is safer than guessing identity from `hands[]` order.

## Disappearance, discontinuity, and time

An eligible slot expires after `handResetTimeoutUs=400,000` without being seen. A finite wrist landmark 0 displacement greater than `teleportThreshold=0.35` in normalized 3D coordinates resets that hand before filtering. Both events reset normalized and world banks together.

All 126 potential channels for a hand receive the exact same frame timestamp converted once to seconds. A repeated/regressive hand timestamp resets its slot. A negative frame timestamp resets all slots and passes raw. Non-finite coordinate samples follow the scalar invalid-sample policy and do not contaminate state.

Filtering defaults to enabled. `setEnabled`/`VisionWorker::setFilteringEnabled` provides local runtime passthrough; changing the flag resets state so re-enabling cannot interpolate from stale observations. No persistent Settings/QSettings integration was added.

## Limitations

Handedness is not a persistent tracking identifier. An UNKNOWN interval intentionally loses smoothing state, and two same-handed observations cannot be associated. The 400 ms timeout and 0.35 teleport threshold are initial safety values and require real-camera tuning. No gesture state machine, action dispatcher, native input, LIVE_STREAM backend, or complex tracker is part of this phase.
