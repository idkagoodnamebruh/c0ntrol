# Phase 2 test report

Environment: Ubuntu 24.04 x86_64, GNU C++ 13.3, CMake 3.28.3, Bazel 6.5.0 selected by Bazelisk, Qt 6.4.2, OpenCV 4.6.0, MediaPipe v0.10.26.

Core clean configure/build/CTest: **6/6 PASS**, total 0.10 s. `make test`: **6/6 PASS**, total 0.09 s.

Real bridge build: **PASS**. Initial real compiler failures were actionable: invalid `hdrs` on `cc_binary`, missing EGL headers, and upstream system-OpenCV include configuration. These were addressed at the bridge/environment boundary; no tracking architecture changed.

MediaPipe integration CMake target compile/link: **PASS**. CTest: **2/2 PASS**, 1.62 s. Neutral test proves Create, two VIDEO calls with microseconds that truncate to the same millisecond, valid inference, and repeated shutdown. Real-hand test proves detection and conversion: `hands=1 handedness=RIGHT score=0.995122 world=21`.

MediaPipe-enabled desktop configure/build: **PASS**. A real Qt MOC compile exposed metatype declaration include ordering; adding the existing Qt bridge include where `Landmarks` first appears in Q_OBJECT headers fixed compilation without behavior change. Camera runtime and live shutdown are **NOT TESTED** because `/dev/video*` is absent.
