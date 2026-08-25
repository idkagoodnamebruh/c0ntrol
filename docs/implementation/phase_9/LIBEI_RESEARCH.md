# libei and liboeffis research

## Authoritative sources

- libei C API: <https://libinput.pages.freedesktop.org/libei/api/libei_8h.html>
- libei sender API: <https://libinput.pages.freedesktop.org/libei/api/group__libei-sender.html>
- liboeffis API: <https://libinput.pages.freedesktop.org/libei/api/group__liboeffis.html>
- XDG RemoteDesktop portal: <https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html>
- exact 1.2.1 source used for compatibility review:
  <https://gitlab.freedesktop.org/libinput/libei/-/tree/1.2.1>

The upstream site currently documents newer releases. The implementation was
therefore checked against the official 1.2.1 source (`meson` version 1.2.1,
checkout commit `08f1d41085a6ae4bac7bc52abe2955d3354342cb`) and compiled by
Ubuntu 24.04 packages version `1.2.1-1`.

## Minimum API used

- `oeffis_new`, `oeffis_create_session`, `oeffis_get_fd`,
  `oeffis_dispatch`, `oeffis_get_event`, `oeffis_get_eis_fd`;
- explicit `OEFFIS_DEVICE_POINTER` only;
- `ei_new_sender`, `ei_configure_name`, `ei_setup_backend_fd`;
- `ei_dispatch`, `ei_get_event`, `ei_event_get_type`;
- seat binding for `EI_DEVICE_CAP_POINTER_ABSOLUTE`,
  `EI_DEVICE_CAP_BUTTON` and `EI_DEVICE_CAP_SCROLL`;
- device added/resumed/paused/removed events;
- indexed regions and their x/y/width/height;
- `ei_device_start_emulating`, `ei_device_stop_emulating`;
- absolute motion, Linux `BTN_LEFT`, discrete scroll, `ei_now` and
  `ei_device_frame`.

Every listed API is present in 1.2.1. `ei_device_get_region_at`, available
since 1.1, is not required; c0ntrol keeps an overflow-safe pure representation
for deterministic tests and monitor-gap validation.

## Build packages

CMake uses the verified pkg-config module names `libei-1.0` and
`liboeffis-1.0`, both with minimum version 1.2.1. Ubuntu CI installs
`libei-dev` and `liboeffis-dev`. A normal desktop build warns and selects the
Null backend if either module is absent; `REQUIRE_LINUX_EIS_INPUT=ON` turns
that condition into a configure error for CI.

## Scroll convention

libei discrete scroll uses 120 units per logical wheel click. The core contract
already follows Windows wheel semantics, where positive logical notches mean
wheel-forward/up. The Linux adapter performs one platform-boundary inversion:
`libeiY = -logicalNotches * 120`. Pure tests cover both signs, zero and integer
overflow.
