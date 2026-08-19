# Gesture baseline before Phase 4

Audited at approved Phase 3 head `8d6fca4810a41854fbe100949717d2d2b6964481`, before changing gesture code.

## Existing static classifier

`GestureClassifier` exposes one stateless `classify(Landmarks)` function and the mutually exclusive enum `NONE`, `UNKNOWN`, `POINTING`, `PINCH`, `PALM_OPEN`, `FIST`, and `VICTORY`. Fewer than 21 points returns NONE.

PINCH has unconditional first priority. It uses MediaPipe thumb tip 4 and index tip 8 and returns PINCH when their absolute normalized-coordinate distance is below `0.05`. The distance is not divided by hand size, so apparent camera scale affects the result.

The remaining poses use only four global image-Y comparisons:

- index: tip 8 versus PIP 6;
- middle: tip 12 versus PIP 10;
- ring: tip 16 versus PIP 14;
- pinky: tip 20 versus PIP 18.

A finger is considered extended exactly when `tip.y < pip.y`. POINTING requires only index extended. VICTORY requires index+middle; PALM_OPEN requires all four; FIST requires none. The thumb is ignored outside pinch. These rules assume fingers point toward decreasing image Y and therefore change under in-plane hand rotation.

## Runtime consumers and repeated click path

`VisionWorker` emits filtered tracking and separately converts the same filtered frame to legacy `Landmarks` for `frameProcessed`. `MainWindow::onFrameProcessed` runs `GestureClassifier` in the GUI thread, passes the resulting enum plus landmarks to `CursorController`, and renders the enum in developer telemetry.

`CursorController::onLandmarksUpdated` maps filtered index tip 8 to primary-screen pixels. POINTING and PINCH move the Qt cursor. Every PINCH frame also calls `performClick(Qt::LeftButton)`, which emits `clickPerformed`; there is no receiver and no native mouse button API, but the per-frame semantic click repetition is still incorrect.

The current code has static geometry only. It has no normalized hand scale, palm-relative features, continuous curl, hysteresis, debounce, candidate states, tracking-loss timeout, or one-shot begin/end events. Those temporal responsibilities belong to the new GestureStateMachine, not the feature extractor or GUI.
