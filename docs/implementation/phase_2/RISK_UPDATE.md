# Phase 2 risk update

| Risk | State | Evidence |
|---|---|---|
| R-001 mock landmarks | RESOLVED | real static-image inference produced 21 landmarks; mock remains explicit fallback/testing |
| R-002 backend absent | RESOLVED | pinned bridge/backend compile, link, Create and both inference tests pass |
| R-003 build | PARTIALLY_RESOLVED | core, MediaPipe target and desktop all build here; reproducibility still requires documented external toolchain |
| R-006 models | RESOLVED | real `HandLandmarker::Create` accepted repository Task bundle |
| R-017 tracking contract | RESOLVED | real C++→C ABI→HandTrackingFrame conversion validated |
| R-025 camera blocking | OPEN | VIDEO inference and OpenCV read can block one QTimer iteration |

R-008/R-009 (OneEuro), R-010/R-011 (gestures), R-012 (FSM), and R-013 (native input) remain OPEN and untouched.
