# Runtime settings schema v2

## New fields

Phase 8 increments `kRuntimeConfigVersion` from 1 to 2. `RuntimeConfig` adds a
Qt-free `DynamicGestureConfig`, while `InputConfig` adds:

- `swipeScrollEnabled` (default true);
- `scrollNotchesPerSwipe` (default 3, valid 1..10);
- `invertSwipeScroll` (default false).

The dynamic group persists enabled, minimum distance, minimum velocity,
direction dominance, maximum duration, maximum sample gap, cooldown and minimum
sample count. Floating-point fields reject NaN and infinity. Distance and
velocity must be positive, dominance must exceed 1, durations/gap must be
positive, cooldown must be nonnegative, and sample count must be at least 2.

## v1 to v2 migration

The decoder accepts schema 1, reads every Phase 7 camera, pointer, filter,
static-gesture and input field, and leaves only the new Phase 8 fields at their
canonical defaults. It returns `migrated=true` and schema 2. `QtSettingsStore`
therefore persists the migrated complete map after loading. Missing, corrupt,
future-schema and unknown-key behavior remains conservative.

Tests construct a real v1 map with all v2 keys absent and prove preservation of
camera, pointer, both filter channels, static gesture configuration, input
enabled state and preferred hand. Full schema-v2 round trip is exact.

## Minimal UI and calibration safety

`SettingsDialog` adds only Enable swipe scrolling, scroll amount and invert
scroll. Opening Settings first suspends native input through
`RuntimeConfigController`, which calls `setInputEnabled(false)` and retries any
previously failed owned-button release. No MOVE, DOWN, UP or SCROLL is dispatched
while Settings or Calibration is modal.

Cancel restores the pre-dialog enabled state without changing the stored
`RuntimeConfig`. Save applies the requested sanitized configuration; Reset
applies canonical schema-v2 defaults. A failed suspension is reported and the
dialog is not opened silently.
