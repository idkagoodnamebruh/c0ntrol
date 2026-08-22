# Windows input research

Research was limited to official Microsoft Learn documentation.

## SendInput contract

[`SendInput`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
accepts an array of `INPUT` values and returns the number successfully inserted.
The backend submits one input and requires a return of exactly one. On mismatch
it records the returned count and `GetLastError`; it does not throw in the hot
path or retry indefinitely.

Microsoft documents that `SendInput` is subject to UIPI: an application can
inject only into applications at an equal or lower integrity level. Neither the
return value nor `GetLastError` identifies UIPI as the cause. c0ntrol reports
failure and never elevates itself automatically.

[`INPUT`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-input)
uses `type=INPUT_MOUSE` and its `MOUSEINPUT` union member. The
[`MOUSEINPUT`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-mouseinput)
flags used are:

- `MOUSEEVENTF_MOVE`, `MOUSEEVENTF_ABSOLUTE`, and
  `MOUSEEVENTF_VIRTUALDESK` for absolute virtual-desktop movement;
- `MOUSEEVENTF_LEFTDOWN` once on confirmed pinch begin;
- `MOUSEEVENTF_LEFTUP` once on end/cancel/safety release.

Microsoft states that absolute `(0,0)` maps to the upper-left and
`(65535,65535)` to the lower-right; `VIRTUALDESK` extends that mapping from the
primary monitor to the entire virtual desktop. Button flags describe
transitions, not a per-frame held condition.

## Virtual desktop and DPI

[`GetSystemMetrics`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getsystemmetrics)
provides `SM_XVIRTUALSCREEN`, `SM_YVIRTUALSCREEN`, `SM_CXVIRTUALSCREEN` and
`SM_CYVIRTUALSCREEN`. The origins can be negative. Microsoft also notes that a
zero return can be a valid metric and `GetLastError` supplies no extended
information; c0ntrol treats only non-positive width/height as invalid.

The official
[`multiple-monitor metrics`](https://learn.microsoft.com/en-us/windows/win32/gdi/multiple-monitor-system-metrics)
page defines these values as the bounding virtual screen. The
[`High DPI desktop development`](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)
guidance warns that DPI-unaware/system-aware contexts may receive virtualized
API values and recommends per-monitor V2 awareness for modern desktop apps.

The Phase 5 backend does not invent logical/physical rescaling. It uses the
same virtual geometry for camera-to-pixel and pixel-to-absolute normalization.
Mixed-DPI physical behavior must be verified in a real Windows session.
