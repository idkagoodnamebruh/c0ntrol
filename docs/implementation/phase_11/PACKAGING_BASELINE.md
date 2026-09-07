# Phase 11 packaging baseline

## Audited source state

Phase 11 starts from the published Phase 10 head
`ce151d9b5bb7fc4f80fd6bc830c9d07453c7c475`. The source tree is clean and
the Phase 11 branch was created directly from that commit.

## Build and runtime behavior before Phase 11

- `BUILD_APP=ON` builds a desktop executable, but there is no production
  build profile and `ENABLE_MEDIAPIPE` defaults to `OFF`.
- A build with the mock tracker can therefore be mistaken for a production
  build. Configuration does not reject that combination.
- The MediaPipe bridge is supplied as an absolute external library path. The
  repository has no reproducible bridge acquisition/build entry point and no
  install rule for the bridge runtime.
- `VisionWorker` inherits the relative default
  `models/hand_landmarker.task`; installed execution consequently depends on
  the current working directory or the source tree.
- The model itself is present and its audited SHA-256 is
  `fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1`.
- The app has no hardware-free `--version` or `--help` path. Starting the
  executable always constructs `QApplication`, the main window, camera path
  and tracking worker.
- CMake discovers application files with recursive globs. This silently
  includes dormant GUI implementations such as `MinimalistModeWidget` and
  `MatrixRainWidget` even though no live code references either widget.
- There are no install rules, CPack configuration, release manifest, staging
  verification, package-integrity test or third-party notices file.

## Runtime dependency inventory

The production executable needs:

- the pinned MediaPipe v0.10.26 bridge built from upstream commit
  `80ae8afbd03465b0d6d9f9e874f8cacf093d23e9`;
- `models/hand_landmarker.task` with the audited hash above;
- Qt 6 Core, Gui and Widgets runtime libraries and platform plugins;
- OpenCV core, imgproc, videoio and video runtime libraries;
- the Windows system runtime and SendInput backend on Windows;
- system Qt/OpenCV plus libei/liboeffis 1.2.1 or newer on production Linux.

User settings are stored outside the package through `QSettings`; they are not
release payload. Tests and their static hand asset are validation inputs, not
runtime package content.

## Existing validation baseline

- Linux Ubuntu 24.04: 25/25 CTest, critical lifecycle stress 100/100,
  ThreadSanitizer 2/2 and desktop compile/link PASS in run `32821192006`, job
  `97719475132`.
- Windows: 22/22 CTest, critical lifecycle stress 100/100 and Windows backend
  compile PASS in run `32821192002`, job `97719475108`.
- Physical camera, portal and native-input smoke are intentionally absent from
  CI.

## Phase 11 boundary

Phase 11 may change build, install, runtime path resolution, release metadata,
packaging tests and documentation. It must not change tracking mathematics,
gesture/FSM behavior, capture architecture, pointer mapping or native input
semantics.
