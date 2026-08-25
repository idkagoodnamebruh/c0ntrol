# Async native input architecture

## Thread and ownership model

```text
Qt GUI thread
  MainWindow
  GesturePipeline
  RuntimeConfigController (sanitize/diff only in GUI use)
       |
       | requestConfiguration / requestEnabled / submitLatest
       v
Native input runtime mutex + bounded handoff
       |
       v
Native input worker (single std::jthread)
  unique_ptr<ISystemInputBackend>
  ActionDispatcher
  platform initialize / geometry / MOVE / DOWN / UP / SCROLL / shutdown

VisionWorker QThread
  VisionWorker
       |
       | queued filteredTrackingFrameProcessed
       v
Qt GUI thread -> GesturePipeline -> submitLatest

AsyncCapture internal capture thread
  unchanged latest-frame camera handoff
```

`MainWindow` no longer owns an `ISystemInputBackend` or
`ActionDispatcher`. It never calls backend initialization or native dispatch.
The backend is moved into the `std::jthread` callable. The worker constructs,
uses and destroys its `ActionDispatcher` and backend there, so libei portal/EIS
objects and Windows `SendInput` operations have one sequential operational
owner.

The platform abstraction intentionally remains synchronous. On Linux,
`LibeiPortalSession::initialize()` may continue waiting for portal permission
inside the input worker. The Qt GUI event loop remains free. A thread-safe
generation and desired-state request allow disable/shutdown to invalidate that
attempt even though the worker cannot service another command until initialize
returns.

## Handoff and latency policy

`submitLatest()` only locks long enough to reject, replace or append bounded
plain data. It cannot execute libei, SendInput or dispatcher code. Movement-only
results occupy one latest-wins slot. Results containing semantic gesture events
occupy a 16-frame bounded deque so edges maintain order. Overflow prefers to
drop non-release work; releases for a hand are coalesced and the newest safety
release is retained.

The worker prioritizes queued semantic frames and then consumes the freshest
movement sample. Disabling, configuration boundaries, failed dispatch and the
READY transition clear both stores. This bounds memory and prevents a camera
backlog from becoming delayed native input.

## Lifecycle and destruction

The runtime owns a joinable `std::jthread`; there is no detach and no callback
capturing `MainWindow`. Shutdown immediately publishes `STOPPING`, rejects new
submissions, clears pending work and requests stop. If platform initialize is
not cancellable, shutdown performs a controlled join after it returns. Its
result cannot publish READY because generation/desired state is checked under
the publication mutex. A READY dispatcher releases an owned primary button
once before backend shutdown.
