#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

scan_paths=(src tests tools scripts CMakeLists.txt Makefile README.md)
marker_pattern='^([<]{7}|[=]{7}|[>]{7})'

if grep -R -I -n -E -- "$marker_pattern" "${scan_paths[@]}"; then
    echo "[ERROR] Git conflict markers found in production/tooling scope." >&2
    exit 1
fi

echo "[PASS] No Git conflict markers in production/tooling scope."
