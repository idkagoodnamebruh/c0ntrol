# Pointer calibration

## Model

`PointerCalibration` is Qt-, OpenCV- and OS-input-free. It estimates the active
normalized camera region and returns the existing `PointerMappingConfig`; it
does not introduce a second mapping path.

The UI collects nine active-pointing samples at the desired top-left and nine
at the desired bottom-right. Samples must have finite x/y/z and normalized x/y
inside [0, 1]. Each boundary is the independent median of its sample set. A
single outlier therefore cannot move a nine-sample boundary.

For medians `left`, `top`, `right`, `bottom`, the result is:

```text
leftMargin   = left
rightMargin  = 1 - right
topMargin    = top
bottomMargin = 1 - bottom
```

The prior mirror-X/mirror-Y flags are preserved. The region must satisfy
`right - left >= 0.10` and `bottom - top >= 0.10`; reversed, undersampled,
non-finite or degenerate calibration returns no result. The caller therefore
retains the previous mapping.

## UI and runtime

The Settings dialog exposes native-input enable, camera index, mirror X,
mirror Y, pointer calibration and reset defaults. Calibration accepts samples
from the currently preferred valid pointing hand while its modal dialog is
active. It never sends OS input.

An accepted result is passed through `RuntimeConfigController`, which releases
a held button before replacing the sole `PointerMapper` configuration. The
result is then persisted. Cancel or invalid calibration changes nothing.

No sensitivity curve, dead zone, second mapper or computer-vision algorithm was
added.
