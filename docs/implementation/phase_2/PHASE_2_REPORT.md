# Phase 2 report

Phase 2 introduces an independent tracking contract, backend interface, explicit mock, optional native MediaPipe C++ Tasks implementation, monotonic timing/frame IDs, raw Qt transport and temporary legacy adapter. VisionWorker now acquires RGB, assigns correlation metadata, submits one frame, emits the raw contract and adapts one selected hand; it no longer contains mock mathematics.

The MediaPipe backend targets Hand Landmarker VIDEO mode, validates the model path, makes an owned row-wise RGB copy, invokes `DetectForVideo`, and converts normalized/world landmarks plus handedness immediately. No vendor type leaves its PIMPL.

Phase 2B corrects the official wrapper access, pins MediaPipe v0.10.26/`80ae8afb`, adds a Bazel-built shared C bridge with its transitive closure, explicit `Close()`, monotonic VIDEO milliseconds, filesystem error handling, and a headless integration target independent of Qt/OpenCV.

Phase 2C closes the previous environment block: official pinned source was obtained by codeload tarball, the real Bazel bridge compiled, both backend integration tests passed, the model was accepted by `Create`, and the official real-hand asset produced one valid hand with 21 normalized and world landmarks plus handedness. The MediaPipe-enabled desktop application also builds. Phase 2 is therefore **COMPLETE**; camera runtime remains unavailable and R-025 remains open.

OneEuro mathematics, GestureClassifier, FSM, dynamic gestures and OS input were not changed. Six headless tests pass. See the research/build/test/risk documents for limitations.
