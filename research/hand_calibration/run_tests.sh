#!/usr/bin/env bash
set -euo pipefail

# GCC/Clang fallback when CMake is unavailable. Builds only the research test.
repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="$(mktemp -d "${TMPDIR:-/tmp}/c0ntrol-hand-calibration.XXXXXX")"
trap 'rm -rf -- "$build_dir"' EXIT

flags=(-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
if [[ "${SANITIZE:-0}" == 1 ]]; then
    flags+=(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
else
    flags+=(-O2 -DNDEBUG)
fi
"${CXX:-c++}" "${flags[@]}" -I "$repo_root" \
    "$repo_root/tests/test_hand_calibration.cpp" \
    "$repo_root/src/core/calibration/HandCalibrationSession.cpp" \
    "$repo_root/src/core/calibration/HandCalibrationStatistics.cpp" \
    -o "$build_dir/test_hand_calibration"
"$build_dir/test_hand_calibration"
