# Latest-frame policy

## Definitions

- **captured**: the source returned one successful frame. Its capture sequence
  and capture-completion timestamp are assigned immediately.
- **published**: that captured frame was placed in `LatestFrameSlot`. Every
  successful capture is published once.
- **overwritten**: publication replaced a frame that had not yet been consumed.
  The counter increments once for that replacement.
- **consumed**: `consumeLatest()` moved the pending frame out and made the slot
  empty.
- **processed**: VisionWorker completed the tracking/filter preparation for a
  consumed frame and recorded pipeline metrics.

The slot invariant is:

```text
pending frames in application memory is either 0 or 1
```

For a producer sequence `100, 101, 102` with no intervening consume, publication
stores 100, replaces it with 101 and then replaces 101 with 102. The consumer
gets only 102 and `overwrittenFrames == 2`. Calling consume again returns empty,
so 102 cannot be processed twice.

`captureSequence` starts at one for each explicit capture start/restart and
increments for every successful source read, including frames later
overwritten. `HandTrackingFrame.frameId` is that capture sequence. Gaps such as
`10, 11, 14` therefore expose that newer frames won over 12 and 13.

The overwrite counter is the authoritative application-slot drop count.
`capturedFrames - processedFrames` is not used as a substitute because it can
also include the currently pending frame or a consumed frame rejected during
later processing. Source read failures are counted separately as
`captureFailures`.

This policy does not depend on `CAP_PROP_BUFFERSIZE`. Phase 6 requests buffer
size 1 as a best effort and records `SUPPORTED`, `NOT_SUPPORTED` or `UNKNOWN`,
but the project slot remains bounded even when OpenCV ignores the property.
