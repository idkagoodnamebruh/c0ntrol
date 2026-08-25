# Phase 10 risk update

## R-006 — RESOLVED

The accepted pinned MediaPipe/model validation is unchanged.

## R-007 — RESOLVED

Phase 9B model hygiene/tooling remains green in Phase 10 Linux CI.

## R-013 — PARTIAL

The real libei/liboeffis 1.2.1 backend and desktop compile/link in Ubuntu
24.04. No physical compositor/portal permission and injected-pointer smoke was
run, so the risk is not closed.

## R-014 — RESOLVED

Windows backend compile and all 22 Windows tests pass. SendInput semantics were
not changed.

## R-015 — PARTIAL

Native smoke remains opt-in and was not run on physical Windows hardware.

## R-021 — PARTIAL

Capture and camera policy are unchanged. Phase 10 only bounds the downstream
native-input handoff and does not claim camera performance closure.

## R-022 — RESOLVED

Input remains disabled by default; first run does not initialize a backend or
open a portal. Automated tests use fakes/Null and produce no native input.

## R-025 — PARTIAL

Linux and Windows CI, deterministic concurrency stress and TSAN pass. Physical
native-input smoke remains absent, so no stronger closure is claimed.

## R-026 — OPEN

The deferred scope associated with this risk was not changed in Phase 10.

## R-027 — OPEN

Packaging is not part of Phase 10.

## R-028 — RESOLVED

No GUI path invokes potentially interactive backend initialization. A
dedicated worker owns backend and dispatcher, request enable returns with
ACTIVATING, generation checks prevent stale READY, pending input is bounded and
pre-READY input is discarded. Linux 25/25, Windows 22/22, 200/200 combined
critical repetitions and TSAN 2/2 validate the lifecycle.
