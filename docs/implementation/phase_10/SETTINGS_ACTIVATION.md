# Settings and native input activation

## Desired versus effective state

The Settings checkbox represents the user's desired configuration.
`NativeInputState::READY` represents effective native input. These values are
not interchangeable:

- first-run default is disabled and creates no portal request;
- checking enable requests `ACTIVATING` without waiting in the GUI;
- a new `enabled=true` is stored only after status polling observes `READY`;
- other settings are saved immediately with `input.enabled=false` while an
  activation is pending;
- failure records the backend error, updates effective runtime configuration to
  disabled and persists false;
- an explicit disable persists false immediately;
- a previously successful persisted opt-in is scheduled with
  `QTimer::singleShot(0, ...)`, after the Qt event loop is alive. The
  `MainWindow` constructor never opens the portal.

The status bar shows Disabled, Activating…, Ready, or Failed with a shortened
real backend error. There is no modal permission-wait dialog.

## Settings suspension

Opening Settings first records whether native input should later be restored
and immediately requests disable. `submitLatest()` becomes non-dispatching at
that request boundary.

- From READY, opening the modal is deferred asynchronously until the worker has
  released any owned button and reached DISABLED.
- From ACTIVATING, no native button can yet be owned. The generation is
  invalidated immediately and the modal may open while the stale platform wait
  finishes; its eventual success is shut down and cannot become READY.
- From FAILED or DISABLED, suspension is a safe no-op and the modal opens.

Camera frames may keep reaching the nested Qt dialog event loop for pointer
calibration, but the runtime rejects MOVE, DOWN, UP and SCROLL throughout the
suspension. Cancelling requests a fresh asynchronous activation only if input
was desired before the modal. Saving applies the newest pointer/input config;
an enable request uses that newest config when READY.

Camera changes occur only after this Settings suspension boundary. A READY
runtime is therefore released before camera restart. An ACTIVATING runtime is
logically cancelled, so it cannot adopt stale configuration during restart.

## Failure and retry

FAILED is visible and stable, not fake READY. It retains `lastError`, emits no
actions and persists false. The user can reopen Settings and enable input to
start a new generation. Null/unsupported platforms follow the same finite
ACTIVATING to FAILED path and remain safe under repeated retries.
