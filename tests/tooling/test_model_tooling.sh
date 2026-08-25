#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
DOWNLOADER="$ROOT_DIR/scripts/download_hand_model.sh"
MODEL_PATH="$ROOT_DIR/models/hand_landmarker.task"

bash -n "$DOWNLOADER"
bash "$ROOT_DIR/scripts/check_repository_hygiene.sh"
bash "$ROOT_DIR/scripts/check_model_assets.sh"

before_sha="$(sha256sum "$MODEL_PATH" | awk '{print $1}')"
C0NTROL_MODELS_DIR="$ROOT_DIR/models" bash "$DOWNLOADER"
after_sha="$(sha256sum "$MODEL_PATH" | awk '{print $1}')"
if [ "$before_sha" != "$after_sha" ]; then
    echo "[ERROR] Existing valid model was unexpectedly replaced." >&2
    exit 1
fi

test_root="$(mktemp -d)"
cleanup() {
    rm -rf -- "$test_root"
}
trap cleanup EXIT HUP INT TERM

failure_dir="$test_root/models"
invalid_url="file://$test_root/does-not-exist.task"
if C0NTROL_MODELS_DIR="$failure_dir" \
    C0NTROL_HAND_LANDMARKER_URL="$invalid_url" \
    bash "$DOWNLOADER" >"$test_root/failure.log" 2>&1; then
    echo "[ERROR] Controlled failed download returned success." >&2
    exit 1
fi

if [ -e "$failure_dir/hand_landmarker.task" ]; then
    echo "[ERROR] Failed download created a final model file." >&2
    exit 1
fi

if find "$failure_dir" -maxdepth 1 -type f -name '.hand_landmarker.task.tmp.*' \
    -print -quit | grep -q .; then
    echo "[ERROR] Failed download left a temporary file." >&2
    exit 1
fi

echo "[PASS] Downloader failure propagates and leaves no placeholder."
