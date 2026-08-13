# Phase 2 risk update

| Risk | State | Evidence |
|---|---|---|
| R-001 mock landmarks | PARTIALLY_RESOLVED | generation moved out of VisionWorker; default remains mock when MediaPipe disabled; no real runtime proof |
| R-002 backend absent | PARTIALLY_RESOLVED | concrete C++ Tasks backend exists; dependency/build/inference unvalidated |
| R-003 build | ENVIRONMENT_BLOCKED | 6/6 core tests pass; app lacks Qt6/OpenCV/MediaPipe |
| R-006 models | PARTIALLY_RESOLVED | Task bundle is non-empty/readable and contains two TFLite assets; inference validation absent |
| R-017 tracking contract | RESOLVED | independent contract and headless tests cover 0/1/2 hands, metadata and adapter |
| R-025 camera blocking | OPEN | VIDEO inference and OpenCV read can block one QTimer iteration |

R-008/R-009 (OneEuro), R-010/R-011 (gestures), R-012 (FSM), and R-013 (native input) remain OPEN and untouched.
