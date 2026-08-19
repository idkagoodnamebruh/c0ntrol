# Canonical scalar One Euro design

## Contract and units

`OneEuroFilter` owns the state of exactly one scalar channel. `filter(value, timestampSeconds)` accepts seconds. `LandmarkFilterBank` performs the only microseconds-to-seconds conversion: `HandTrackingFrame.timestampUs / 1,000,000.0`. Every axis in a frame therefore receives one identical timestamp; no per-axis clock calls exist.

Configuration is explicit:

| Parameter | Initial default | Meaning |
|---|---:|---|
| `minCutoff` | 1.0 Hz | minimum signal low-pass cutoff |
| `beta` | 0.05 | motion-adaptive cutoff gain |
| `derivativeCutoff` | 1.0 Hz | derivative low-pass cutoff (`dCutoff`) |
| `maxDeltaSeconds` | 1.0 s | scalar safety gap that forces reset |

These are initial defaults, not camera-tuned constants. Non-finite/invalid configuration values are replaced with safe defaults; beta may be zero but not negative.

## Equations

For a valid positive `dt`:

```text
dx = (x - previousRaw) / dt
filteredDx = lowPass(dx, alpha(derivativeCutoff, dt))
cutoff = minCutoff + beta * abs(filteredDx)
filteredX = lowPass(x, alpha(cutoff, dt))

tau = 1 / (2*pi*cutoff)
alpha(cutoff, dt) = 1 / (1 + tau/dt)
```

The derivative references the previous raw sample, not the previous filtered sample. `derivativeCutoff` is actively applied to the derivative low-pass filter before adaptive cutoff calculation.

## State and reset semantics

The first finite sample with a finite timestamp initializes the signal state to the sample and derivative state to zero, then returns the sample unchanged. `reset()` clears both low-pass filters, the previous raw value, timestamp, and initialized flag.

Repeated or regressive timestamps, `dt<=0`, or `dt>maxDeltaSeconds` reset and initialize from the current finite sample, returning raw for that sample. A non-finite timestamp resets and returns raw without initializing because it cannot establish a valid time origin.

A NaN/Inf value never changes raw, time, derivative, or signal state. It returns the last valid filtered output; before any valid sample it returns finite neutral `0.0`. A later valid sample continues from the last valid timestamp.

## Channel isolation

The scalar class has no `filterPoint` API. One hand requires 21 landmarks × 3 axes = 63 objects. Normalized and world coordinates use different 63-object banks, so neither axes, landmarks, coordinate spaces, nor LEFT/RIGHT hands share signal or derivative history.
