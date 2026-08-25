# Native input lifecycle baseline

## Approved base

Phase 10 starts from published Phase 9B head
`5f18a16af90323a467b0a2a70f8c934c05c80db1` on
`implementation/phase-9-linux-eis-input`.

## Actual call and thread ownership before Phase 10

`main()` constructs `MainWindow` on the Qt GUI thread. Its constructor creates
the platform backend and `ActionDispatcher`, then calls
`ActionDispatcher::initialize()` on that same GUI thread. Disabled first-run
configuration returns without touching the backend, but a persisted
`input.enabled=true` reaches `ensureBackendInitialized()`,
`ISystemInputBackend::initialize()` and, on supported Wayland builds,
`LibeiPortalSession::initialize()` before `app.exec()` starts.

`VisionWorker` and `AsyncCapture` run outside the GUI thread, but the queued
`filteredTrackingFrameProcessed` signal is delivered to
`MainWindow::onFilteredTrackingFrameProcessed()` on the GUI thread. The GUI
thread executes `GesturePipeline::process()` and then synchronously calls
`ActionDispatcher::process()`. Native MOVE/DOWN/UP/SCROLL therefore also run on
the GUI thread.

Opening Settings calls `RuntimeConfigController::suspendInput()` on the GUI
thread. That calls `ActionDispatcher::setInputEnabled(false)` and potentially
`releaseAll()`. Saving or cancelling can call `setInputEnabled(true)` or
`applyConfiguration()` synchronously from the same thread. Camera-change and
reset paths call `releaseAll()` there as well.

`MainWindow::~MainWindow()` calls `ActionDispatcher::shutdown()` on the GUI
thread before synchronously stopping the vision worker. Thus initialize,
process, release and shutdown are sequential today, but all native input work
is owned by the GUI thread.

## Blocking window

`LibeiPortalSession::initialize()` creates the pointer-only RemoteDesktop
session and polls portal/EIS file descriptors in 100 ms increments. Its portal
deadline is 120 seconds. Once ConnectToEIS arrives it allows up to 10 seconds
for resumed absolute-pointer, button and scroll devices with valid regions,
still bounded by the original portal deadline. Permission interaction,
compositor response and device discovery can therefore hold the GUI path for
up to roughly two minutes.

## Cancellation and queued frames before Phase 10

There is no activation generation or thread-safe desired-state channel.
Because the GUI thread is blocked inside initialize, a disable request from the
same event loop cannot run until initialization returns. A late successful
activation can therefore become enabled before the queued disable is handled.

While the GUI event loop is blocked, the vision/capture side can continue
producing queued Qt signal deliveries. The dispatcher has timestamp replay
protection once calls resume, but there is no explicit activation-boundary
flush and no bounded native-input handoff slot. This does not satisfy the
requirement that pre-READY gestures can never be replayed after READY.

## Settings and persistence before Phase 10

First-run defaults are disabled and do not open the portal. A successful
Settings save is synchronous: the controller applies the requested enabled
state before `ISettingsStore::save()`. Failure prevents the whole apply/save.
A persisted opt-in is attempted in the `MainWindow` constructor, so denial can
also delay initial window creation.

## Phase 10 boundary

R-028 — **OPEN**: potentially interactive native backend initialization is
reachable from the GUI thread. Phase 10 must introduce a Qt-free dedicated
runtime that owns both backend and dispatcher, explicit lifecycle state,
generation cancellation and a bounded post-READY handoff. MediaPipe, filters,
gesture mathematics, pointer mapping and platform event semantics remain
unchanged.
