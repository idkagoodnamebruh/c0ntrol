# Capture baseline

## Audited state

This baseline is the source tree at the approved Phase 5 head
`8a6509e18952a629f6bff90217cbbc857c5ab04c`.

- `VisionWorker` constructed a `QTimer` with a 33 ms interval.
- `VisionWorker::start()` opened camera index 0 and requested 640 x 480 through
  a `cv::VideoCapture` member.
- The `VideoCapture` was owned by `VisionWorker`, which lives on its dedicated
  Qt worker thread after `moveToThread()`.
- Every timer callback called `m_cap >> frame` in `processFrame()` on that same
  worker thread. A backend or driver delay could therefore block the Qt event
  loop that also executes inference and stop requests.
- BGR-to-RGB conversion and MediaPipe processing followed the read in the same
  callback.
- `TrackingClock` generated a steady-clock timestamp at processing time, not at
  capture completion. Its frame ID counted processed callbacks, so skipped
  camera frames were not observable.
- The project had no application-level camera queue, capacity limit, explicit
  overwrite policy or overwrite counter. Any buffering inside OpenCV or the
  camera backend was opaque.
- If processing exceeded 33 ms, no independent producer could continue placing
  a newer frame in a bounded slot. Qt timer delivery did not provide an
  application-level latest-frame policy.
- `MainWindow` passed the literal `30.0` to developer telemetry for every
  processed frame. This was a requested/assumed rate, not a measurement.
- `DeveloperModeWidget` appended wrist text per frame, and `MainWindow` logged
  gesture events in the per-frame path.
- Shutdown invoked `VisionWorker::stop()` on the worker thread, stopped the
  timer, shut down tracking, reset filters and released `VideoCapture`. A
  blocking read could delay delivery of that stop invocation.

## OpenCV facts used for the redesign

Only official OpenCV documentation was used:

- [`VideoCapture::read`, `grab`, `retrieve`, `release` and
  `getBackendName`](https://docs.opencv.org/4.x/d8/dfe/classcv_1_1VideoCapture.html)
- [Video I/O property definitions](https://docs.opencv.org/4.x/d4/d15/group__videoio__flags__base.html)

`read()` combines grab and retrieve and reports failure through `false`/an
empty image. `release()` closes the device. Property behavior passes through
several layers and is backend, driver and hardware dependent. Width, height,
FPS and buffer size are therefore requests rather than guarantees.

OpenCV documents `CAP_PROP_OPEN_TIMEOUT_MSEC` and
`CAP_PROP_READ_TIMEOUT_MSEC` as open-only properties for FFmpeg and GStreamer.
The project opens a camera through `CAP_ANY`, so Phase 6 does not claim a
portable read timeout and does not set one after opening.
