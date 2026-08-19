# Tracking contract

`HandTrackingTypes.h` is independent of Qt, OpenCV and MediaPipe. `TrackedHand` always owns exactly 21 normalized points, optionally 21 world points, handedness and its classification score. `HandTrackingFrame` owns 0–2 expected hands, a monotonic microsecond timestamp, monotonic frame ID and validity flag. Valid inference with zero hands is distinct from a failed/invalid result.

`RgbImageView` is a synchronous borrowed view used only for a `process()` call. Concrete backends must copy if they retain input. `IHandTrackingBackend` owns initialize/process/shutdown/error semantics without leaking vendor types.

Hand order from MediaPipe is explicitly **not stable identity**. Persistent IDs are future work. The temporary legacy adapter deterministically prefers RIGHT, otherwise the first hand, and returns empty landmarks for invalid/zero-hand frames. This selection is compatibility policy, not permanent tracking architecture.

`TrackingClock` uses `steady_clock`, forces strictly increasing microseconds, and allocates frame IDs from zero. `HandTrackingFrame` is declared and registered through the Qt-only bridge for raw worker→GUI observation.

The concrete VIDEO backend converts microseconds to milliseconds and enforces strict increase even when two microsecond values truncate to the same millisecond. Its C ABI is an implementation boundary only and never enters domain/GUI types.

Real inference validated the complete conversion chain (official C++ result → C ABI → domain): one hand, 21 finite normalized points, RIGHT/0.995122 handedness, and 21 finite world points. Zero-hand validity remains covered by neutral inference/core semantics; hand order remains non-identity.
