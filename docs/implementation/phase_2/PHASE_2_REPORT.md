# Phase 2 report

Phase 2 introduces an independent tracking contract, backend interface, explicit mock, optional native MediaPipe C++ Tasks implementation, monotonic timing/frame IDs, raw Qt transport and temporary legacy adapter. VisionWorker now acquires RGB, assigns correlation metadata, submits one frame, emits the raw contract and adapts one selected hand; it no longer contains mock mathematics.

The MediaPipe backend targets Hand Landmarker VIDEO mode, validates the model path, makes an owned row-wise RGB copy, invokes `DetectForVideo`, and converts normalized/world landmarks plus handedness immediately. No vendor type leaves its PIMPL.

This phase is **PARTIAL / ENVIRONMENT_BLOCKED**: concrete MediaPipe headers/library, Qt6 and OpenCV were unavailable, so the real backend was not compiled or run and real landmarks cannot be claimed. Mock/default fallback is explicit. The model bundle structure was validated, not its inference behavior.

OneEuro mathematics, GestureClassifier, FSM, dynamic gestures and OS input were not changed. Six headless tests pass. See the research/build/test/risk documents for limitations.
