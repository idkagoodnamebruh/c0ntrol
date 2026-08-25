#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MODEL_PATH="$ROOT_DIR/models/hand_landmarker.task"
EXPECTED_SHA256="fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1"
MIN_MODEL_BYTES=1048576

if [ ! -f "$MODEL_PATH" ]; then
    echo "[ERROR] Required runtime model is missing: $MODEL_PATH" >&2
    exit 1
fi

model_size="$(wc -c < "$MODEL_PATH" | tr -d '[:space:]')"
if [ "$model_size" -lt "$MIN_MODEL_BYTES" ]; then
    echo "[ERROR] Runtime model is too small ($model_size bytes)." >&2
    exit 1
fi

if command -v sha256sum >/dev/null 2>&1; then
    actual_sha256="$(sha256sum "$MODEL_PATH" | awk '{print $1}')"
elif command -v shasum >/dev/null 2>&1; then
    actual_sha256="$(shasum -a 256 "$MODEL_PATH" | awk '{print $1}')"
else
    echo "[ERROR] sha256sum or shasum is required." >&2
    exit 1
fi

if [ "$actual_sha256" != "$EXPECTED_SHA256" ]; then
    echo "[ERROR] Runtime model SHA-256 mismatch." >&2
    exit 1
fi

zero_byte_models="$(find "$ROOT_DIR/models" -maxdepth 1 -type f -size 0 -print)"
if [ -n "$zero_byte_models" ]; then
    echo "[ERROR] Zero-byte model files are not allowed:" >&2
    echo "$zero_byte_models" >&2
    exit 1
fi

echo "[PASS] Canonical runtime model: $model_size bytes, SHA-256 $actual_sha256"
