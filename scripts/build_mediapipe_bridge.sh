#!/usr/bin/env bash
set -euo pipefail

readonly MEDIAPIPE_TAG="v0.10.26"
readonly MEDIAPIPE_COMMIT="80ae8afbd03465b0d6d9f9e874f8cacf093d23e9"

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <empty-work-directory> <output-directory>" >&2
  exit 2
fi

work_directory="$1"
output_directory="$2"
script_directory="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repository_root="$(cd "${script_directory}/.." && pwd)"
source_directory="${work_directory}/mediapipe"

if [[ -e "${source_directory}" ]]; then
  echo "refusing to overwrite existing MediaPipe checkout: ${source_directory}" >&2
  exit 1
fi

mkdir -p "${work_directory}" "${output_directory}"
git clone --branch "${MEDIAPIPE_TAG}" --depth 1 \
  https://github.com/google-ai-edge/mediapipe.git "${source_directory}"

actual_commit="$(git -C "${source_directory}" rev-parse HEAD)"
if [[ "${actual_commit}" != "${MEDIAPIPE_COMMIT}" ]]; then
  echo "MediaPipe commit mismatch: ${actual_commit}" >&2
  exit 1
fi

cp -R "${repository_root}/third_party/mediapipe_bridge" \
  "${source_directory}/c0ntrol_bridge"
cp "${repository_root}/third_party/mediapipe_patches/opencv_linux.BUILD" \
  "${source_directory}/third_party/opencv_linux.BUILD"

if command -v bazelisk >/dev/null 2>&1; then
  bazel_command="bazelisk"
elif command -v bazel >/dev/null 2>&1; then
  bazel_command="bazel"
else
  echo "bazelisk or bazel is required" >&2
  exit 1
fi

(
  cd "${source_directory}"
  "${bazel_command}" build -c opt \
    --repo_env=HERMETIC_PYTHON_VERSION=3.12 \
    //c0ntrol_bridge:libc0ntrol_mediapipe_bridge.so
)

bridge="${source_directory}/bazel-bin/c0ntrol_bridge/libc0ntrol_mediapipe_bridge.so"
test -f "${bridge}"
cp "${bridge}" "${output_directory}/"
echo "MEDIAPIPE_SOURCE=${source_directory}"
echo "MEDIAPIPE_COMMIT=${actual_commit}"
echo "MEDIAPIPE_BRIDGE=${output_directory}/libc0ntrol_mediapipe_bridge.so"
