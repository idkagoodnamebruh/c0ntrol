# Pointer mapping

## Camera to virtual desktop

`PointerMapper` is the canonical runtime transformation. The input is filtered
landmark 8 in normalized camera coordinates. Non-finite points are rejected;
finite X/Y values are clamped to `[0,1]`.

`PointerMappingConfig` exposes independent `mirrorX` and `mirrorY` flags. Both
default to false. Mirroring is applied exactly once after active-region
normalization and is independent of GUI preview rendering.

The active region uses normalized left/right/top/bottom margins. Defaults are
zero because no camera tuning was available. For one axis:

`active = clamp((clamp(value,0,1) - lowMargin) / (1-low-high), 0, 1)`.

Invalid or overlapping margins are replaced with safe zero margins. The pixel
mapping preserves virtual-desktop origins, including negative coordinates:

`pixel = origin + round(active * (size - 1))`.

Tests cover center, corners, clamp, both mirror axes, margins, non-zero and
negative origins, multimonitor dimensions, NaN and invalid geometry.

## Pixel to Win32 absolute

The OS backend converts a clamped desktop pixel to `[0,65535]` using the
virtual origin and `size-1`:

`absolute = round((pixel-origin) * 65535 / (size-1))`.

It sends the result with `MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE |
MOUSEEVENTF_VIRTUALDESK`, so 0 and 65535 address the complete virtual desktop
rather than only the primary monitor.

No extra DPI scale factor is invented. The backend consumes virtual-screen
metrics in the process coordinate context and normalizes them consistently.
Microsoft notes that some APIs can be virtualized for non/per-system-aware
threads, so physical-pixel behavior across mixed-DPI monitors remains part of
the unavailable Windows integration validation rather than a claimed result.
