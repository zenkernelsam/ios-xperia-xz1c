#!/usr/bin/env bash
set -euo pipefail
project=$(cd "$(dirname "$0")/../.." && pwd)
version=${1:-725}
case "$version" in 725|726|727|728|729) ;; *) echo "Supported versions: 725, 726, 727, 728, 729" >&2; exit 2 ;; esac
python3 "$project/tools/build/build_diagnostic_kernel.py" --version "$version"
