# XDG RemoteDesktop portal lifecycle

## Activation

With the default `InputConfig.enabled=false`, dispatcher initialization is
logical only and the native backend initialization count remains zero. No
portal object, D-Bus session or consent prompt is created.

An explicit false-to-true transition, or startup with a previously enabled
setting, performs:

1. `oeffis_new()`;
2. `oeffis_create_session(..., OEFFIS_DEVICE_POINTER)`;
3. liboeffis `CreateSession` / `SelectDevices` / `Start`;
4. mandatory user consent presented by the portal;
5. `RemoteDesktop.ConnectToEIS` and duplicated EIS fd;
6. `ei_new_sender` plus `ei_setup_backend_fd`;
7. seat binding and asynchronous device/region discovery;
8. backend READY only after all required active capabilities and valid regions
   exist.

Keyboard and touchscreen permissions are never requested. No attempt is made
to bypass or persist portal consent.

## Failure and retry

Portal cancellation, denial, timeout, D-Bus disconnect, invalid regions or
missing EIS capabilities returns a concrete `lastError`. The dispatcher keeps
runtime input disabled and forces its operational `enabled` value false, so
Settings cannot persist a successful enabled state for a failed activation.
A later explicit enable creates/reinitializes the session and may retry.

## Runtime and shutdown

After connection, the portal is retained only to keep the authorized session
alive and observe close/disconnect events. MOVE, DOWN, UP and SCROLL do not use
D-Bus; they travel through libei/EIS. One GUI/runtime thread owns the portal,
libei context and devices, matching the existing dispatcher call path.

Shutdown first releases dispatcher ownership, then attempts a backend-owned
BTN_LEFT UP, stops active emulation sequences, unreferences devices and libei,
and finally unreferences liboeffis to close the portal session. Repeated
shutdown is safe.
