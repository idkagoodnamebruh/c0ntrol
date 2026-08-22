# Threading and shutdown

## Ownership table

| Object | Owner/execution context | Blocking work |
| --- | --- | --- |
| `MainWindow`, `ActionDispatcher` | GUI thread | none added |
| `VisionWorker`, tracking/filter pipeline | Qt vision thread | inference only |
| `AsyncCapture` producer | one `std::jthread` | source `open/read/close` |
| `OpenCVCameraSource::VideoCapture` | capture producer only | backend-dependent read |
| `LatestFrameSlot` | shared producer/consumer | short replace/move lock |

## Start

VisionWorker initializes tracking, resets filters and metrics, and starts the
producer. The producer opens the source, records requested/observed camera
properties and enters `RUNNING`. The 5 ms consumer timer only begins after the
producer launch. Startup logging occurs once when RUNNING is observed.

## Normal processing

The producer blocks in the camera source as appropriate, timestamps and
sequences each successful read, and publishes it. VisionWorker atomically takes
the newest pending frame or returns without work. No Qt signal is emitted for
each captured frame.

## Normal stop and app destruction

The order is:

```text
ActionDispatcher::shutdown (safe native button release)
  -> blocking VisionWorker::stop invocation
  -> stop consumer timer
  -> request capture stop
  -> producer leaves read/backoff
  -> producer closes/releases camera
  -> jthread join
  -> tracking shutdown/filter reset
  -> Qt worker thread quit/join
```

`start()` and `stop()` are idempotent. The producer is joined before its source
or slot can be destroyed, and the stopped consumer timer cannot emit another
frame after `VisionWorker::stop()` returns.

## Error and destruction paths

An open failure or terminal read failure retains an error and enters `FAILED`.
VisionWorker rate-limits repeated tracking errors to once per second and reports
a terminal capture error once. It then stops the consumer, joins capture and
shuts tracking down. Destruction calls `stop()` again safely.

The fake camera's blocking read waits on a condition variable and
`requestStop()` wakes it, proving the interface contract and clean synthetic
join. The real OpenCV source does not manipulate `VideoCapture` from a second
thread. Consequently a misbehaving driver read may delay stop until it returns;
with no accessible camera, Phase 6 does not claim driver-level cancellation.
