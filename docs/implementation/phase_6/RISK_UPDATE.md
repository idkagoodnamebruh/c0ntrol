# Phase 6 risk update

## R-020 — RESOLVED

The fixed `30.0` FPS value was removed from MainWindow telemetry. Capture and
processing rates now derive from steady-clock samples, metric history is
bounded, developer updates are capped at 5 Hz and per-frame text/event logging
was removed. The remaining `30.0` is clearly a requested camera configuration,
not an observed rate.

## R-021 — PARTIALLY_RESOLVED

Application-level camera buffering is bounded to one latest frame, slow
consumers overwrite stale frames, an empty slot cannot duplicate work,
BGR-to-RGB runs only for selected frames, and telemetry/logging work is
throttled. The 250-frame synthetic stress case validates those invariants.

Real camera plus MediaPipe profiling is unavailable, so Phase 6 makes no claim
about achieved end-to-end throughput, driver buffering, sensor latency or CPU
usage in production.

## R-025 — PARTIALLY_RESOLVED

`VideoCapture::read()` no longer runs in VisionWorker's Qt event loop. A
dedicated producer owns open/read/release and hands VisionWorker only the newest
frame through a capacity-one slot. Synthetic cooperative blocking cancellation,
stop/join, failure and restart pass on Linux and Windows.

No real camera was available. OpenCV and driver reads remain backend-dependent,
and the project deliberately avoids releasing `VideoCapture` concurrently from
a second thread. A driver that does not return from `read()` may still delay
join. R-025 therefore remains PARTIALLY_RESOLVED until real-device shutdown and
restart behavior are observed.

## Unchanged scope

MediaPipe, OneEuroFilter, LandmarkFilterBank, HandFeatureExtractor,
GestureEngine, GestureStateMachine, ActionDispatcher, PointerMapper and
WindowsSystemInputBackend were not functionally modified. Phase 7 has not
started.
