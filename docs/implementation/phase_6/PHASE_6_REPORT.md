# Phase 6 report

## Status

Phase 6 is **COMPLETE** for the defined asynchronous-capture scope on approved
Phase 5 head `8a6509e18952a629f6bff90217cbbc857c5ab04c`.

The blocking camera read was removed from the VisionWorker event loop. A
dedicated producer owns OpenCV capture and publishes BGR frames into an explicit
latest-frame slot of logical capacity one. VisionWorker consumes at most one new
frame, converts it once, and gives tracking the captured frame's strict
steady-clock timestamp and capture sequence as frame ID.

## Delivered

- testable `ICameraSource`, `AsyncCapture` and `LatestFrameSlot` core;
- OpenCV source with single-thread `VideoCapture` ownership;
- requested 640 x 480 at 30 FPS and buffer size 1, all best effort;
- observed backend/resolution/FPS/buffer reporting after open;
- captured, processed, overwritten and failure counters;
- real sliding-window capture and processing FPS;
- frame-age, inference and processing-duration metrics;
- 5 Hz developer telemetry without fixed FPS or per-frame log growth;
- structured terminal capture handling and rate-limited tracking errors;
- fake camera support for finite, delayed, failed and cancelable-blocking reads;
- capacity, concurrency, dropping, lifecycle, timestamp and metrics tests.

## Validation

- Linux core: 16/16 PASS (previous 13/13 preserved).
- Windows CI implementation run `32560820110`: 17/17 PASS.
- Windows backend compile/link regression: PASS.
- Linux Qt/OpenCV desktop build: PASS.
- Real camera: NOT AVAILABLE (`/dev/video*` absent).
- MediaPipe real integration: not rerun; its implementation was not modified.

The synthetic 250-frame stress case captures sequence 1 through 250, observes
at least one overwrite, processes fewer frames than captured, and consumes the
final sequence. Runtime camera counts cannot be reported without a real device.

## Honest limitations

The capture timestamp is read-completion time, not exposure time. OpenCV
properties are backend-dependent; buffer support is only reported as supported
when the observed value confirms the request. No portable camera read timeout
is assumed. The fake source proves cooperative cancellation, while a real
driver that blocks indefinitely may still delay shutdown. Consequently R-025
and R-021 remain partially resolved pending real camera/MediaPipe profiling.

R-020 is resolved. Phase 7 has not started.
