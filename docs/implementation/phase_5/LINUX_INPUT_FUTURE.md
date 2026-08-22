# Future Linux input contract

Linux native input is not implemented in Phase 5. A future backend will
implement `ISystemInputBackend`, leaving `GesturePipeline`, `ActionDispatcher`
and `PointerMapper` unchanged.

For Wayland, investigate libei/EIS and the compositor/portal-mediated
connection required by the target desktop environment. Permission, session
negotiation and user consent belong in the platform backend/factory, not core.
Generic uinput is not treated as a shortcut for Wayland desktop UX.

If X11 support is still required later, it should be a distinct backend with
its own capability and security model. Do not mix X11 injection into the
Wayland backend.

Until then, `NullSystemInputBackend` returns unsupported from initialization
and every action. It exists to keep Linux builds safe, not to claim Linux input
support or resolve R-013.
