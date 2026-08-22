# Phase 6 handoff

## Stable contract

The runtime capture path is now:

```text
OpenCVCameraSource producer -> LatestFrameSlot(1) -> VisionWorker
  -> tracking -> LandmarkFilterBank -> GesturePipeline -> ActionDispatcher
```

Do not replace the slot with an unbounded frame signal or FIFO. Every successful
read receives a capture sequence and steady-clock read-completion timestamp.
Overwriting a pending frame increments `overwrittenFrames`; consuming empties
the slot, preventing duplicate processing. `HandTrackingFrame.frameId` is the
capture sequence and may legitimately contain gaps.

`VideoCapture` open/read/release belong to the producer thread. Keep locks away
from read, color conversion and inference. Keep BGR-to-RGB in the consumer so
discarded frames are not converted.

## Configuration and telemetry

The default camera request is index 0, 640 x 480, 30 FPS and buffer size 1.
These are requests only. Continue to distinguish requested values from observed
backend values and never display requested FPS as measured FPS.

Capture and processing rates use a bounded one-second/240-sample window.
Developer telemetry is throttled to 5 Hz. Timing names deliberately describe
read completion, frame age and code-section duration rather than sensor or
camera-to-photon latency.

## Validation state

- Linux core: 16/16 PASS.
- Previous Linux core regression: 13/13 PASS.
- Windows CI: 17/17 PASS on implementation run `32560820110`.
- Linux desktop without MediaPipe: PASS with Qt/OpenCV 5.0.0.
- Camera: NOT AVAILABLE.
- R-020: RESOLVED.
- R-021: PARTIALLY_RESOLVED.
- R-025: PARTIALLY_RESOLVED.

## Future work, not Phase 6

Before resolving R-025, test real camera open, continuous capture, disconnect,
stop, restart and shutdown across actual OpenCV backends. Record requested and
observed properties plus capture/processing/overwrite metrics. Specifically
measure how long a blocked real `read()` takes to return after stop request.

Real MediaPipe/camera profiling is also required before resolving R-021. Complex
automatic reconnect, persistent settings, calibration, new gestures and Phase 7
remain out of scope and have not started.
