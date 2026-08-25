#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="${C0NTROL_MODELS_DIR:-$ROOT_DIR/models}"
MODEL_NAME="hand_landmarker.task"
MODEL_PATH="$MODELS_DIR/$MODEL_NAME"
MODEL_URL="${C0NTROL_HAND_LANDMARKER_URL:-https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task}"
EXPECTED_SHA256="fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1"
MIN_MODEL_BYTES=1048576

file_size() {
    wc -c < "$1" | tr -d '[:space:]'
}

file_sha256() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | awk '{print $1}'
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | awk '{print $1}'
    else
        echo "[ERROR] sha256sum or shasum is required to validate $MODEL_NAME." >&2
        return 1
    fi
}

validate_model() {
    local candidate="$1"
    local size
    local actual_sha256

    if [ ! -f "$candidate" ]; then
        echo "[ERROR] Model is not a regular file: $candidate" >&2
        return 1
    fi

    size="$(file_size "$candidate")"
    if [ "$size" -lt "$MIN_MODEL_BYTES" ]; then
        echo "[ERROR] Model is too small ($size bytes; minimum $MIN_MODEL_BYTES): $candidate" >&2
        return 1
    fi

    actual_sha256="$(file_sha256 "$candidate")"
    if [ "$actual_sha256" != "$EXPECTED_SHA256" ]; then
        echo "[ERROR] SHA-256 mismatch for $candidate" >&2
        echo "[ERROR] Expected: $EXPECTED_SHA256" >&2
        echo "[ERROR] Actual:   $actual_sha256" >&2
        return 1
    fi
}

if validate_model "$MODEL_PATH" 2>/dev/null; then
    echo "[INFO] $MODEL_NAME is already present and valid."
    exit 0
fi

mkdir -p "$MODELS_DIR"
temporary_file="$(mktemp "$MODELS_DIR/.${MODEL_NAME}.tmp.XXXXXX")"

cleanup() {
    if [ -n "${temporary_file:-}" ]; then
        rm -f -- "$temporary_file"
    fi
}
trap cleanup EXIT HUP INT TERM

echo "[INFO] Downloading $MODEL_NAME from the official MediaPipe model source."
if command -v curl >/dev/null 2>&1; then
    if ! curl --fail --location --show-error \
        --retry 3 --retry-delay 1 --connect-timeout 15 \
        --output "$temporary_file" "$MODEL_URL"; then
        echo "[ERROR] Download failed; the existing model was not changed." >&2
        exit 1
    fi
elif command -v wget >/dev/null 2>&1; then
    if ! wget --tries=3 --timeout=15 \
        --output-document="$temporary_file" "$MODEL_URL"; then
        echo "[ERROR] Download failed; the existing model was not changed." >&2
        exit 1
    fi
else
    echo "[ERROR] Neither curl nor wget is available." >&2
    exit 1
fi

validate_model "$temporary_file"
mv -f -- "$temporary_file" "$MODEL_PATH"
temporary_file=""
trap - EXIT HUP INT TERM

echo "[SUCCESS] $MODEL_NAME installed atomically at $MODEL_PATH."
