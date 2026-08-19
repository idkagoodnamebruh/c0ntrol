# One Euro baseline before Phase 3

Audited at Phase 2 head `6535e2248b540c175bb5b22e958cce6533360500`, before changing the filter.

## Existing state and equations

`src/core/filters/OneEuroFilter.h` contains both `LowPassFilter` and `OneEuroFilter` as header-only classes. `OneEuroFilter` stores one signal low-pass state, one derivative low-pass state, a mutable frequency, `minCutoff`, `beta`, `dCutoff`, and a last timestamp. Its alpha calculation is `1 / (1 + tau / te)`, where `tau = 1 / (2*pi*cutoff)` and `te = 1/frequency`.

For each sample it derives frequency from a positive timestamp delta, reads the last filtered signal as the derivative reference, computes `(x - previousFiltered) * frequency`, passes that derivative through `m_dxFilter`, raises the signal cutoff by `beta * abs(filteredDerivative)`, and filters the new signal sample. The derivative filter alpha is left at its constructor value of `1.0`; `m_dCutoff` is stored but never read. Consequently R-009 is present in source, not inferred from documentation.

## Initialization and invalid time behavior

The first-sample derivative check is `m_xFilter.lastValue() == 0.0`. Zero is therefore used simultaneously as a legitimate signal value and as an initialization sentinel. `LowPassFilter` itself initializes on its first call, but does not expose or reset that state. There is no public reset or initialized query.

When both timestamps are supplied and `dt > 0`, frequency becomes `1/dt`. A repeated or regressive timestamp silently retains the previous frequency, while `m_lastTime` is still overwritten. The default timestamp `-1` also overwrites `m_lastTime`. There is no explicit policy for non-finite timestamps, large gaps, NaN, or infinity, so those values can contaminate stored filter state.

## Call sites and state contamination

At this Phase 2 head, repository search finds no active `filter()` or `filterPoint()` call outside `tests/test_one_euro.cpp`. `VisionWorker.h` still includes the header, but `VisionWorker.cpp` sends backend output directly to `toLegacyLandmarks`; therefore the real MediaPipe pipeline is currently raw and unfiltered.

The class nevertheless exposes `filterPoint`, which calls the same scalar `filter` three consecutive times with the same timestamp for X, Y, and Z. That is one signal and derivative history for all three axes, so X affects Y and Z. Reusing that object across the 21 landmarks would likewise make every point affect the next. The retired pre-Phase-2 worker pattern used one `OneEuroFilter`, so it provided one state rather than the required 63 scalar states per hand. No independent world-landmark bank exists.

Phase 3 replaces this ambiguous point API with a canonical scalar-only filter and introduces explicit, persistent banks for every landmark axis.
