# Swipe scroll contract

## Semantic boundary

```text
GestureEvent SWIPE_UP / SWIPE_DOWN
  -> ActionDispatcher SCROLL_VERTICAL (signed logical notches)
  -> ISystemInputBackend::scrollVertical(notches)
  -> platform-native wheel units
```

`SWIPE_LEFT` and `SWIPE_RIGHT` remain semantic events. They do not produce an
OS action, shortcut, key or navigation command in Phase 8.

The default mapping is:

- `SWIPE_UP` -> positive logical notches;
- `SWIPE_DOWN` -> negative logical notches.

`invertSwipeScroll` reverses both signs. `scrollNotchesPerSwipe` defaults to 3
and is sanitized to 1..10. Core does not contain the Windows value 120.

## Dispatch policy

At most one vertical swipe is dispatched per processed timestamp. The preferred
hand wins; if it has no vertical swipe, one valid event from the other nominal
hand may be used. Duplicate/regressive timestamps are ignored by the existing
dispatcher replay guard.

No native scroll is sent when master input is disabled or
`swipeScrollEnabled` is false. If the dispatcher owned PRIMARY_BUTTON_DOWN at
the start of the frame, scroll is suppressed even if a safety transition also
releases the button in that frame. This prevents drag and scroll from mixing.
Backend failure is returned through `ActionDispatchResult`; no successful
command is recorded.

## Windows and unsupported platforms

`WindowsSystemInputBackend` multiplies logical notches by `WHEEL_DELTA`, places
the signed delta in `MOUSEINPUT::mouseData`, sets `MOUSEEVENTF_WHEEL` and calls
`SendInput`. It verifies that exactly one input was inserted and preserves the
existing UIPI diagnostic. It does not use `mouse_event`.

This follows Microsoft Learn's [MOUSEINPUT documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-mouseinput)
and [SendInput contract](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
The null backend returns an explicit unsupported error and never pretends that
scroll succeeded. Linux native scrolling remains outside Phase 8.
