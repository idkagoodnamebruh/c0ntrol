# Third-party notices

This file records the direct runtime components used by c0ntrol packages. It
is an inventory, not a replacement for each dependency's license text and not
legal advice. Release producers must preserve notices and license files emitted
by the dependency deployment tools.

## MediaPipe

- Component: MediaPipe Hand Landmarker C++ runtime, pinned to v0.10.26 commit
  `80ae8afbd03465b0d6d9f9e874f8cacf093d23e9`.
- Upstream: https://github.com/google-ai-edge/mediapipe
- Declared upstream license: Apache License 2.0.
- The package also contains Google's official `hand_landmarker.task` model
  asset. Its exact provenance and checksum are recorded in
  `docs/implementation/phase_9/MODEL_ASSET_AUDIT.md`.

## Qt 6

- Components: Qt Core, Gui, Widgets and the platform/runtime dependencies
  selected by the official `windeployqt` tool on Windows.
- Upstream: https://www.qt.io/licensing/
- Qt offers commercial and open-source licensing options. Open-source Qt 6
  modules can carry LGPLv3 or GPLv3 terms, and Qt contains separately licensed
  third-party code. The release producer must use and distribute Qt under the
  terms applicable to the exact Qt build being packaged.

## OpenCV

- Components: OpenCV core, imgproc, videoio and video.
- Upstream: https://github.com/opencv/opencv
- Current OpenCV 4.x declared license: Apache License 2.0; older incorporated
  code also retains notices identified by OpenCV's license-change notice.

## libei and liboeffis (Linux system dependencies)

- Components: libei-1.0 and liboeffis-1.0, version 1.2.1 or newer.
- Upstream: https://gitlab.freedesktop.org/libinput/libei
- Linux packages do not bundle these libraries. Their exact distribution
  copyright files remain supplied by the operating-system packages.

## Project license status

At the Phase 11 baseline this repository does not contain a top-level project
license file. This notice does not invent or grant a license for c0ntrol's own
source code. A release owner should resolve that policy before distributing a
public binary release.
