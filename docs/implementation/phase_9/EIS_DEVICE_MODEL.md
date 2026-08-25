# EIS device and region model

## Capabilities

The sender binds only absolute pointer, button and scroll capabilities. READY
requires all three, but they may be supplied by different EIS devices. Device
records retain one libei reference, capability flags, resume/emulation state
and immutable regions captured after `EI_EVENT_DEVICE_ADDED`.

On `DEVICE_RESUMED`, the device starts a new monotonically sequenced emulation
transaction. On pause it becomes unavailable and a later resume starts a new
transaction. On removal its reference is dropped immediately. Operations pump
pending EIS/portal events first and fail cleanly while a required capability is
paused, removed or disconnected.

## Regions

`EisRegion` uses signed 64-bit x/y/width/height so union calculations can reject
overflow before converting to the core integer `DesktopGeometry`. Empty,
zero-sized, negative-sized, endpoint-overflowing and core-range-overflowing
layouts are invalid.

The existing `PointerMapper` receives the bounding union. Immediately before
absolute motion, the backend verifies the mapped `DesktopPoint` belongs to at
least one real EIS region. A point in a gap between separated monitors returns
`false` with `absolute pointer point is outside all EIS regions`; no snapping or
silent relative fallback is used. The real session then chooses the resumed
absolute device whose own region contains the point.

## Event frames

Absolute motion uses `ei_device_pointer_motion_absolute`. Primary button uses
Linux `BTN_LEFT`. Scroll uses `ei_device_scroll_discrete`. Each generated event
is completed on the same device with `ei_device_frame(device, ei_now(context))`,
providing the required CLOCK_MONOTONIC-compatible microsecond timestamp.
