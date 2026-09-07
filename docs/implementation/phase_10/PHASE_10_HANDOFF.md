# Phase 10 handoff

## Approved lineage and publication

- base branch: `implementation/phase-9-linux-eis-input`
- exact base SHA: `5f18a16af90323a467b0a2a70f8c934c05c80db1`
- Phase 10 branch: `implementation/phase-10-async-input-lifecycle`
- draft PR: [#10](https://github.com/idkagoodnamebruh/c0ntrol/pull/10)
- merge: not performed

## Runtime contract for later phases

- Treat READY as the only native-input-capable state.
- Submit camera/gesture results through `NativeInputRuntime::submitLatest()`;
  do not call platform backends or `ActionDispatcher` from GUI/vision threads.
- Use `requestConfiguration()` for the newest complete pointer/input config and
  `requestEnabled()` for suspension boundaries.
- Do not persist a new enabled=true until READY. Persist false on explicit
  disable or FAILED.
- Any modal/calibration/camera restart must first invalidate admission. From
  READY, wait asynchronously for DISABLED so an owned button has been released.
- Preserve the one-worker backend/dispatcher ownership rule. Libei session,
  refresh, device events and shutdown are sequential on that worker.
- Keep the latest slot capacity one and semantic storage bounded. Do not replace
  it with an unbounded per-camera-frame queue.
- Shutdown must join; never detach the input worker.

## Evidence to retain

- final published head: `ce151d9b5bb7fc4f80fd6bc830c9d07453c7c475`;
- Linux implementation run `32821192006`, job `97719475132`: 25/25, critical
  100/100, TSAN 2/2 and desktop PASS with libei/liboeffis 1.2.1.
- Windows implementation run `32821192002`, job `97719475108`: 22/22 and
  critical 100/100 PASS.
- Physical Wayland and physical Windows smokes were not run and must not be
  inferred from CI.

## Unchanged boundaries

MediaPipe and its model, OneEuro, LandmarkFilterBank, GestureEngine/FSM,
DynamicGestureRecognizer, AsyncCapture, pointer mapping mathematics and native
backend event semantics are unchanged. Phase 11 has not started.
