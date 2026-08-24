# Phase 8 report

## Status

Phase 8 is **COMPLETE** on approved Phase 7 head
`c1b63fb6f2f8127cc6308569ecd25d151a074023`.

## Delivered

- removal of `DynamicGestureTracker` and its frame-count timing dependency;
- Qt/OpenCV/MediaPipe-free `DynamicGestureRecognizer` using real timestamps;
- stabilized `palmCenter` motion normalized by median `handScale`;
- distance, velocity, direction-dominance and OPEN_HAND requirements;
- independent LEFT/RIGHT histories with conservative ambiguity resets;
- tracking-loss, invalid-time, gap and cooldown lifecycle handling;
- timestamped one-shot SWIPE_LEFT/RIGHT/UP/DOWN events in `GesturePipeline`;
- one extraction of `HandFeatures` shared by static and dynamic recognition;
- observable, capacity-12 runtime event buffer;
- configurable vertical swipe scroll with preferred-hand and drag safety;
- `ISystemInputBackend::scrollVertical` and Windows SendInput/WHEEL adapter;
- schema-v2 persistence and explicit v1 migration;
- safe Settings/Calibration modal input suspension;
- expanded deterministic core and desktop-build validation.

Linux core passed 19/19, preserving every Phase 7 target. Linux desktop linked
with Qt/OpenCV. Windows workflow run 32788434408 built the wheel backend and
manual smoke executable and passed 20/20 tests. The smoke executable was not
run and no native event was emitted by CI.

Horizontal swipes remain semantic only. Native smoke execution and physical
camera/hardware tuning were not performed. Phase 9 has not started.
