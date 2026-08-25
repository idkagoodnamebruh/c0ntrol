# Phase 10 test report

## Published implementation under test

- repository: `idkagoodnamebruh/c0ntrol`
- base: `5f18a16af90323a467b0a2a70f8c934c05c80db1`
- implementation validation head:
  `38af85314d7fba75fd0ef76c5be868365d87ec29`
- PR: [#10](https://github.com/idkagoodnamebruh/c0ntrol/pull/10)

The local Windows host did not expose CMake or a compiler. No local PASS is
claimed. Compilation and execution evidence below comes from the published
GitHub Actions head.

## Deterministic async coverage

`FakeBlockingSystemInputBackend` coordinates initialize and movement with
condition variables; deadlines are watchdogs only. The two portable tests
cover disabled startup, non-blocking enable, visible ACTIVATING, success,
failure/error, retry, disable during activation, stale generation, shutdown
during activation, Null failure, no dispatch outside READY, no pre-READY
replay, first post-READY movement, newest config, exact release on disable and
shutdown, double-disable suppression, timestamp replay protection, bounded
queue pressure and safety-release preservation.

## Linux Ubuntu 24.04

[Run 32820611608](https://github.com/idkagoodnamebruh/c0ntrol/actions/runs/32820611608),
job `97717696191`, run number 5:

- real `libei-1.0` 1.2.1 and `liboeffis-1.0` 1.2.1: detected;
- repository/model tooling: PASS;
- strict production libei compile/link and factory test: PASS;
- full CTest: **25/25 PASS** (the prior 23 targets plus two Phase 10 targets);
- `test_native_input_lifecycle`: PASS;
- `test_async_input_runtime`: PASS;
- critical repetition: **100/100 PASS** (each of two tests 50 times);
- GCC 13.3 ThreadSanitizer build and execution: **2/2 PASS**;
- Qt6/OpenCV desktop with real libei/liboeffis: **PASS**.

No test initializes the production portal backend or emits native input.

## Windows

[Run 32820611718](https://github.com/idkagoodnamebruh/c0ntrol/actions/runs/32820611718),
job `97717696622`, run number 18:

- MSVC configure/build: PASS;
- full CTest: **22/22 PASS** (the prior 20 targets plus two Phase 10 targets);
- Windows backend compile target: PASS;
- critical repetition: **100/100 PASS**;
- physical/native smoke: NOT RUN.

## Physical evidence

- physical Wayland portal/EIS input: NOT RUN;
- physical Windows SendInput smoke: NOT RUN;
- MediaPipe inference: not repeated because MediaPipe, model, filters and
  tracking were unchanged.
