# Phase 9 handoff

## Stable Linux input contract

Keep portal creation behind explicit input enablement. The first run defaults
to disabled and must not show RemoteDesktop. Continue requesting only pointer;
keyboard and touchscreen are outside the current action contract.

Do not replace libei hot-path events with D-Bus Notify calls. Preserve absolute
motion, per-device region membership, conservative monitor-gap failure,
Linux `BTN_LEFT`, 120-unit discrete scroll and `ei_now`/device-frame pairing.
Capabilities may live on separate devices and must be treated as unavailable
while paused or removed.

`IEisInputSession` is intentionally small. Unit tests must remain independent
of D-Bus, Wayland, portal availability and real input. The real compile target
must never call `initialize()` in CI.

## Dependency and factory policy

Minimum supported libei and liboeffis version is 1.2.1. A normal Linux desktop
build may warn and fall back to Null when development packages are missing;
strict CI must use `REQUIRE_LINUX_EIS_INPUT=ON`. Do not expose libei headers in
core.

## Model and tooling contract

Keep `models/hand_landmarker.task` as the sole runtime model. Its versioned
official URL, size and SHA-256 are recorded in `MODEL_ASSET_AUDIT.md`. Do not
restore loose detector files or empty placeholders. Any future downloader
change must preserve temporary download, validation, atomic replacement and a
non-zero failure status. Keep the tooling test offline.

## Remaining evidence

On a controlled physical Wayland session, explicitly enable input, accept and
cancel the pointer-only portal in separate runs, inspect reported devices and
regions, validate MOVE/DOWN/UP/SCROLL, remove/revoke the session and confirm
defensive release and clean shutdown. Record compositor, portal backend,
libei version, monitor topology and scale. Also retain the pending physical
Windows and camera validations tracked by R-013/R-015/R-021/R-025.

The portal activation path is synchronous today. Phase 10 may design
non-blocking/asynchronous native-input activation plus observable runtime input
status, but that work is intentionally not part of Phase 9B.

X11 and Phase 10 are not started.
