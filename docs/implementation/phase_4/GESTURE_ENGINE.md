# Gesture engine

`GestureEngine` is deterministic and stateless. It maps one valid
`HandFeatures` value plus frame metadata to one `GestureObservation` and does
not call Qt, OpenCV, MediaPipe or OS APIs.

## Static recognition

- POINTING: index extended and middle/ring/pinky curl each at least `0.38`.
- OPEN_HAND: at least three of the four long fingers extended, unless the
  POINTING rule already matches. The thumb is deliberately tolerant.
- PINCH signal: finite `pinchRatio <= 0.25`.
- NONE: no supported static rule matches.

The static pose enum is only a presentation label. `pointerActive` and
`pinchActive` are separate fields; a pointing hand can therefore pinch without
losing pointer activity. The display priority is POINTING, OPEN_HAND, PINCH,
then NONE, while the temporal FSM consumes the independent pinch ratio.

No synthetic confidence value is reported. Invalid features or negative
timestamps produce an invalid observation.

## Runtime integration

`GesturePipeline` consumes the filtered frame emitted by `VisionWorker`.
Unique LEFT and RIGHT hands are processed independently and events are appended
in deterministic LEFT-then-RIGHT order. UNKNOWN or duplicate handedness is
treated as missing for nominal state, preventing cross-hand contamination.

`MainWindow` selects RIGHT when present, otherwise LEFT, for temporary legacy
pointer display/movement. It only formats telemetry and logs semantic events;
all feature and temporal decisions remain in core.
