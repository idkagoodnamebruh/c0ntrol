# Settings storage

## Boundary

`ISettingsStore` is a Qt-free contract with `load`, `save` and
`resetToDefaults`. `InMemorySettingsStore` supports deterministic core tests.
`QtSettingsStore` is the desktop adapter and is the only layer that includes
`QSettings`.

The application sets organization and application names to `c0ntrol` and stores
encoded keys under the `runtime` group in the platform-native QSettings
location. No global mutable settings singleton exists.

## Schema 1

The map includes `configVersion=1` and all fields for camera, pointer, normalized
and world filters, gestures and input. Floating-point values are serialized with
round-trip precision and the classic locale. Booleans accept only `true/false`
or `1/0`; integers and doubles must parse completely; non-finite doubles are
invalid.

Loading behavior is:

- no group/first run: safe defaults, then persist schema 1;
- missing keys: retain present known values and fill missing fields from
  defaults;
- corrupt or out-of-range fields: sanitize, produce one summarized warning and
  persist the repaired config;
- version 0 or missing version: read known keys, migrate and persist version 1;
- future version: reject interpretation, use disabled-input defaults and issue
  one warning;
- unknown keys: ignored by the core decoder.

Storage access/format errors never crash startup. Main receives safe defaults
and logs one `[SETTINGS]` warning. Saving or resetting calls `sync()` and reports
QSettings access/format failure to the settings dialog.

Reset is not a partial calibration reset: it releases input through the runtime
controller, restores the entire canonical `RuntimeConfig`, persists it, hot-
applies safe components and performs a camera restart only when the camera
request changed.
