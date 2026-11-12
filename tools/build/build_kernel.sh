#!/usr/bin/env bash
set -euo pipefail
export HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_INSTALL_CLEANUP=1
project=$(cd "$(dirname "$0")/../.." && pwd)
python3 "$project/tools/build/prepare_sources.py" kernel
printenv KDKROOT >/dev/null || { echo "Set KDKROOT to the installed KDK_13.5_22G74.kdk directory" >&2; exit 1; }
cd "$project/build/dxb"
export MACOS_VERSION=13.5 KERNEL_CONFIG=DEVELOPMENT ARCH_CONFIG=ARM64 MACHINE_CONFIG=SWEET
# No --clean. The complete reconstructed XNU is already applied.
python3 - "$PWD/build.sh" <<'PY'
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
start=s.index("patches() {");end=s.index("\nbuild_bootstrap_cmds()",start)
if "LILAC_RECONSTRUCTED_SOURCE" not in s[start:end]:
    s=s[:start]+"patches() {\n    info 'LILAC_RECONSTRUCTED_SOURCE: complete pinned patch already applied'\n}\n"+s[end:]
    p.write_text(s)
PY
bash build.sh
