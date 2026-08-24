# Dynamic gesture baseline

## Phase 7 implementation

The legacy implementation was the header-only
`src/core/gestures/DynamicGestureTracker.h`. It stored a
`std::vector<Point3D>` capped by `maxHistory`; the default was ten frames. Each
`addFrame` copied only landmark 0 (wrist), and `detectGesture` compared the first
and last stored wrist positions after the history reached exactly that count.

The default absolute threshold was 0.15 normalized-image units. Horizontal or
vertical classification required only that the selected absolute axis exceed
that threshold and the other axis. A successful detection cleared history.

The tracker had no:

- timestamp or real-duration window;
- velocity requirement;
- hand-scale normalization;
- palm or stabilized feature input;
- handedness identity or independent LEFT/RIGHT state;
- pose gate;
- cooldown;
- explicit tracking-loss or timestamp-gap handling;
- runtime consumer in `GesturePipeline` or `MainWindow`.

`tests/test_dynamic_gestures.cpp` constructed a five-frame tracker with a 0.10
threshold, fed wrist x positions 0.1 through 0.5 at a constant y, and asserted
only `SWIPE_RIGHT`. No other direction, timing, scale, pose or lifecycle case
was covered.

## Phase 8 disposition

The legacy header was removed. `DynamicGestureRecognizer` now consumes the same
`HandFeatures` extracted for static classification and uses timestamps as its
only temporal definition. No frame-count timing model remains.
