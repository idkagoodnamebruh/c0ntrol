# Pipeline metrics

## Counters and rates

`PipelineMetrics` exposes:

- capture FPS and processing FPS;
- captured, processed, overwritten and capture-failure counters;
- frame age at processing start;
- inference duration and total processing duration;
- current capture state.

Capture FPS uses successful read-completion timestamps. Processing FPS uses
processing-completion timestamps. Both use `SlidingWindowRate`: at most 240
samples and approximately the latest one-second window are retained. With at
least two samples, the rate is:

```text
(sample_count - 1) * 1,000,000 / elapsed_microseconds
```

The first sample reports 0 rather than inventing a target FPS. No sample history
is unbounded.

## Timing definitions

- `captureTimestampUs`: `steady_clock` immediately after a successful source
  read. It is read-completion time, not sensor exposure time.
- `frameAgeAtProcessingUs`: processing start minus capture completion.
- `inferenceDurationUs`: return from tracking minus entry to tracking.
- `processingDurationUs`: processing end minus processing start, including
  conversion, tracking, filtering preparation and owned GUI-image creation.

Durations are clamped to nonnegative values. The timestamp passed to MediaPipe
derives from capture completion and is forced forward by one microsecond only
if necessary to preserve VIDEO-mode strict monotonicity.

## Presentation

Developer mode labels capture and processing rates separately and shows counts,
frame age, inference time and processing time. VisionWorker emits a metrics
snapshot at most every 200 ms (5 updates per second), regardless of capture or
processing rate. The previous fixed `30.0` telemetry and per-frame text log were
removed. The requested camera FPS remains 30 as configuration and is never
presented as a measurement.
