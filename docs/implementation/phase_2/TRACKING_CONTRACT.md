# Tracking contract

`HandTrackingTypes.h` is independent of Qt, OpenCV and MediaPipe. `TrackedHand` always owns exactly 21 normalized points, optionally 21 world points, handedness and its classification score. `HandTrackingFrame` owns 0–2 expected hands, a monotonic microsecond timestamp, monotonic frame ID and validity flag. Valid inference with zero hands is distinct from a failed/invalid result.

`RgbImageView` is a synchronous borrowed view used only for a `process()` call. Concrete backends must copy if they retain input. `IHandTrackingBackend` owns initialize/process/shutdown/error semantics without leaking vendor types.

Hand order from MediaPipe is explicitly **not stable identity**. Persistent IDs are future work. The temporary legacy adapter deterministically prefers RIGHT, otherwise the first hand, and returns empty landmarks for invalid/zero-hand frames. This selection is compatibility policy, not permanent tracking architecture.

`TrackingClock` uses `steady_clock`, forces strictly increasing microseconds, and allocates frame IDs from zero. `HandTrackingFrame` is declared and registered through the Qt-only bridge for raw worker→GUI observation.
