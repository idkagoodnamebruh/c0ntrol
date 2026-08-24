# Phase 8 handoff

## Stable dynamic contract

Keep `DynamicGestureRecognizer` Qt-, OpenCV- and MediaPipe-free. Feed it the
`HandFeatures` already extracted by `GesturePipeline`; do not add a second
landmark geometry path. All timing must remain in `timestampUs`, and movement
thresholds must remain in hand-scale units.

Do not weaken OPEN_HAND gating to make tests or demos easier. POINTING and PINCH
must reset dynamic history, nominal hand histories must never transfer, and
UNKNOWN/duplicate identities must remain conservative. A physical movement may
emit one event, then must start a fresh post-cooldown window.

The defaults (1.25 scales, 4.0 scales/s, dominance 1.5, 500 ms window, 150 ms
gap, 400 ms cooldown, three samples) are initial deterministic defaults. Tune
only from recorded physical-camera evidence and preserve schema migration.

## Stable action contract

Core `SCROLL_VERTICAL` carries signed logical notches. Only the Windows adapter
translates notches to `WHEEL_DELTA`; do not leak Win32 constants upward.
Horizontal swipes have no OS action. Master input disable, swipe-scroll disable,
preferred-hand selection, one-scroll-per-frame and held-button suppression are
safety boundaries, not optional UX choices.

Opening Settings or Calibration must suspend native input before entering a
modal loop. Cancel restores the previous enabled state without persisting a
change; Save applies the requested config through `RuntimeConfigController`.

## Remaining evidence

On controlled physical Windows hardware, run the existing opt-in smoke and a
separately confirmed wheel check, record UIPI and monitor/DPI context, and tune
dynamic thresholds with a real filtered MediaPipe stream. Physical-camera
profiling and disconnect/shutdown remain required for R-021/R-025. Linux native
input, horizontal OS actions and Phase 9 remain out of scope.
