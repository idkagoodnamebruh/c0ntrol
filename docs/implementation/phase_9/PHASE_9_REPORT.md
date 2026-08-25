# Phase 9 report

## Status

Phase 9 is **COMPLETE** on approved Phase 8 head
`05d0060a2b2aeb31c8da29c48376497d1941ad30`.

## Delivered

- lazy native backend initialization with safe failure and retry;
- XDG RemoteDesktop lifecycle through liboeffis, requesting pointer only;
- libei 1.2.1 sender context using the ConnectToEIS fd;
- asynchronous seat, device, capability and region discovery;
- separate resumed devices for absolute pointer, button and scroll;
- absolute movement with real-region and monitor-gap checks;
- Linux `BTN_LEFT` DOWN/UP and backend-owned defensive release;
- discrete vertical scroll with tested 120-unit/sign translation;
- pause/remove/disconnect failure and resume recovery;
- overflow-safe multiple-region desktop geometry;
- Linux factory selection and Null fallback;
- optional dependency warning plus strict CI dependency mode;
- fake-session/pure unit tests and non-interactive real-library compile test;
- Ubuntu 24.04 strict CI and real-library Linux desktop build;
- inherited dynamic cooldown hardening against a one-frame pose flicker.

## Validation summary

- local dependency-free Linux: **21/21 PASS**;
- critical repetitions: **80/80 PASS**;
- Ubuntu 24.04 real libei/liboeffis 1.2.1: **22/22 PASS**;
- production Linux backend compile/link and factory selection: **PASS**;
- Linux desktop linked against real libraries: **PASS**;
- Windows regression: **20/20 PASS**;
- physical Wayland portal smoke: **NOT RUN**.

No test or workflow opens the portal or emits real input. Physical validation
remains explicit in R-013 rather than being represented as complete evidence.
No root, uinput, XTest, X11 or keyboard permission is used. Phase 10 has not
started.
