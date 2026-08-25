# Model asset audit

## Source search

Phase 9B searched exact references to `hand_landmarker.task`,
`hand_detector.tflite`, `hand_landmarks_detector.tflite` and
`hand_landmark.onnx` in `src/`, `tests/`, `tools/`, `scripts/`, CMake, the
Makefile, README and current implementation documentation.

Only `hand_landmarker.task` has product consumers:

- `HandTrackingConfig::modelPath` defaults to
  `models/hand_landmarker.task`;
- both real MediaPipe integration tests receive that path from CMake;
- the desktop backend passes the default config to
  `MediaPipeHandTrackingBackend` when `ENABLE_MEDIAPIPE=ON`.

References to the other names were confined to the broken downloader and
historical inventory/closure documents. A historical statement is not a
runtime, test or tool consumer.

## Asset decisions

| Path at the Phase 9 base | Base size | Runtime consumer | Test consumer | Required | Legacy | Phase 9B action |
|---|---:|---|---|---|---|---|
| `models/hand_landmarker.task` | 7,819,105 B | Yes | Yes, real MediaPipe tests | Yes | No | Retained as the sole canonical model; size and SHA-256 validated |
| `models/hand_detector.tflite` | 2,339,878 B | No | No | No | Yes | Removed; its role is already packaged inside the task bundle |
| `models/hand_landmarks_detector.tflite` | 5,478,949 B | No | No | No | Yes | Removed; its role is already packaged inside the task bundle |
| `models/hand_landmark.onnx` | 0 B | No | No | No | Yes | Removed; invalid placeholder with no consumer |

The current `models/` directory consequently contains one regular file and no
zero-byte assets.

## Canonical model identity

- file: `models/hand_landmarker.task`;
- size: 7,819,105 bytes;
- SHA-256:
  `fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1`;
- official versioned URL:
  `https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task`;
- official model documentation:
  <https://developers.google.com/edge/mediapipe/solutions/vision/hand_landmarker#models>.

The Google documentation identifies Hand Landmarker as a single model bundle
containing both palm detection and hand-landmark detection models. Phase 2
additionally proved that this exact checked-in asset is accepted by
`HandLandmarker::Create` and produces real 21-point inference through the
pinned MediaPipe v0.10.26 bridge.

## Download and CI contract

`scripts/download_hand_model.sh` downloads only the canonical bundle. It uses a
temporary in the destination directory, checks a conservative 1 MiB minimum
and the fixed SHA-256 associated with the versioned URL, then performs an
atomic `mv`. `C0NTROL_MODELS_DIR` and
`C0NTROL_HAND_LANDMARKER_URL` exist for controlled tooling tests; they do not
change the official default.

Download failures propagate as non-zero, the temporary is removed by a trap,
and the final path is never replaced until validation succeeds. No operation
creates an empty placeholder. Linux CI runs Bash syntax, repository conflict
marker, canonical model and controlled failure-path checks without downloading
the 7.8 MB bundle.
