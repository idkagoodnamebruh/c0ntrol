# Native input state machine

`NativeInputRuntime` exposes a Qt-free, thread-safe `NativeInputStatus`. A
state is effective, not aspirational: only `READY` permits submission to reach
the dispatcher. `generation` changes on enable, disable and shutdown
boundaries, and a worker may adopt `READY` only while its captured generation
still matches the desired generation under the runtime mutex.

| Current | Request/event | Next | Native events allowed? | Persist enabled? | Notes |
|---|---|---|---|---|---|
| `DISABLED` | enable | `ACTIVATING` | No | No | Caller returns immediately; worker starts synchronous backend initialization. |
| `ACTIVATING` | current-generation success and stable latest config | `READY` | Yes, for later submissions only | Yes | Pending slots and dispatcher temporal state are reset before adoption. |
| `ACTIVATING` | initialization/configuration failure | `FAILED` | No | No | Real backend error is retained; desired/effective enable becomes false. |
| `ACTIVATING` | disable | `STOPPING`, then `DISABLED` | No | No | Desired state and generation change immediately. Late success is shut down and discarded. |
| `READY` | disable | `STOPPING`, then `DISABLED` | No after the request | No | Pending work is cleared; an owned button is released exactly once before backend shutdown. |
| `READY` | shutdown | `STOPPING`, then `DISABLED` | No after the request | Unchanged | Application shutdown joins the worker; a previously successful opt-in may remain persisted for the next asynchronous startup. |
| `FAILED` | retry enable | `ACTIVATING` | No | No | A new generation performs a new initialize attempt. |
| `FAILED` | disable | `DISABLED` | No | No | Clears the failure as an explicit disabled state. |
| `STOPPING` | newer enable | `ACTIVATING` desired state | No | No | In-flight older work remains stale; the worker tears it down before the new attempt. |
| any | shutdown | `STOPPING`, then `DISABLED` | No | Unchanged | No detached worker and no callback into destroyed GUI state. |

`desiredEnabled` is protected by the same mutex as `generation`, state,
configuration and pending slots. It is therefore recorded even while the
worker is blocked inside a platform `initialize()` call. The worker checks it
again before publishing either a failure or `READY`.

## Event admission

- `DISABLED`, `ACTIVATING`, `FAILED` and `STOPPING` reject every submitted
  movement and semantic event.
- The `ACTIVATING` to `READY` boundary clears all pending work, so a pinch,
  swipe or pointer sample produced before permission cannot replay later.
- `READY` uses one capacity-one latest pointer slot plus at most 16 semantic
  frames. Latest movement overwrites older movement. Semantic overflow drops
  or coalesces older work while retaining a safety release preferentially.
- `ActionDispatcher` remains the only owner of active-hand, timestamp,
  duplicate-DOWN and defensive-UP semantics.
