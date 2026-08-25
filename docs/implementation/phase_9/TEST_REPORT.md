# Phase 9 test report

## Linux core and fake-session validation

A clean WSL/Arch configuration with GCC 16.2.1 and CMake 4.4.2 used
`BUILD_APP=OFF`, `BUILD_TESTING=ON`, `ENABLE_MEDIAPIPE=OFF` and
`ENABLE_LINUX_EIS_INPUT=OFF`. This exercises the dependency-free core and the
fake-session Linux logic without a portal: **22/22 PASS** on the local Phase 9B
implementation. All 19 Phase 8 Linux
targets remain present and pass; `test_eis_regions` and
`test_linux_eis_backend` raise the Phase 9 total to 21, and
`test_model_tooling` raises the Phase 9B total to 22.

The tooling target passed Bash syntax, conflict-marker and canonical-model
checks. It verified 7,819,105 bytes and SHA-256
`fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1`,
confirmed an existing valid model is not redownloaded, and proved a controlled
offline failure returns non-zero without a final or temporary model file.

The dynamic recognizer, action dispatcher, EIS regions and Linux fake backend
were each repeated 20 times: **80/80 executions PASS**. Coverage includes lazy
init/retry/reuse, complete and missing capabilities, adjacent and separated
regions, monitor gaps, overflow, inside/outside motion, positive/negative
scroll, button idempotence, pause/remove failure, resume recovery,
disconnect, defensive release and double shutdown. The inherited pose-flicker
case proves cooldown cannot be bypassed.

The production translation unit also passed `-Wall -Wextra -Werror -pedantic`
syntax compilation directly against the exact official libei/liboeffis 1.2.1
headers.

## Ubuntu 24.04 real-library CI

Fresh Phase 9B workflow evidence is pending publication. The run below is
retained explicitly as pre-9B backend evidence and must not be interpreted as
validating the Phase 9B tooling commit.

GitHub Actions run `32792725170` (run number 1), job `97637451558`, validated
implementation head `fefc340285b51e94b16fbb81fb5cc5af83c92d28` on explicit
`ubuntu-24.04`:

- installed `libei-dev` and `liboeffis-dev` version 1.2.1;
- strict CMake configure with both enable/require options: **PASS**;
- production `LibeiPortalSession.cpp` compile/link: **PASS**;
- factory selected `LinuxEisSystemInputBackend`: **PASS**;
- CTest step, 22 registered targets (previous 19 plus three Linux targets):
  **22/22 PASS**;
- Qt6/OpenCV desktop configure and complete link with the real libraries:
  **PASS**.

The compile test only constructs/destroys the backend. CI never calls
`initialize()`, opens RemoteDesktop or emits input.

## Windows regression

Fresh Phase 9B workflow evidence is pending publication. The run below is
retained explicitly as pre-9B backend evidence and must not be interpreted as
validating the Phase 9B tooling commit.

GitHub Actions run `32792725125` (run number 14), job `97637451423`, completed
configure, MSVC build and CTest: **20/20 PASS**. Win32 SendInput behavior was not
changed and the native smoke executable was not run.

## Desktop and physical smoke

A separate local Linux Qt/OpenCV 5.0.0 fallback build linked `c0ntrol`: **PASS**.
The Ubuntu workflow additionally linked the desktop against real
libei/liboeffis: **PASS**. No physical Wayland portal/compositor smoke was
available, so native Linux smoke is **NOT RUN** and no physical input event was
sent during validation.
