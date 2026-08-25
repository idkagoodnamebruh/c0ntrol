# Lazy native input activation

`ActionDispatcher::initialize()` now initializes dispatcher state first. It
calls `ensureBackendInitialized()` only when the sanitized startup config is
already enabled. Safe first-run defaults therefore never enter an interactive
platform flow.

`setInputEnabled(true)` initializes the backend at most once, installs valid
desktop geometry and only then exposes `inputEnabled()==true`. Failure clears
the operational enabled flag and retains `lastError`. Another enable retries.
Disabling releases an owned button but intentionally keeps a successful native
session available for re-enable.

`applyConfiguration()` performs the same readiness check before committing an
enabled input config. `RuntimeConfigController` updates and Settings persistence
occur only after that method succeeds. MainWindow reflects a failed persisted
startup opt-in as disabled runtime state.

Tests prove:

1. disabled startup initializes the native backend zero times;
2. first enable initializes exactly once;
3. denied initialization leaves input disabled;
4. a second enable retries successfully;
5. valid geometry is installed before mapping;
6. disable releases BTN_LEFT;
7. re-enable reuses the live session;
8. shutdown is safe whether native initialization happened or not.

Windows still initializes at enabled startup and reuses its backend on
disable/re-enable; its remote regression workflow remains green.
