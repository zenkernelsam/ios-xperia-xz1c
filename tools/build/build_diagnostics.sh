#!/usr/bin/env bash
set -euo pipefail
project=$(cd "$(dirname "$0")/../.." && pwd)
stage="$project/build/surface-diag-v725"
test -f "$stage/candidate.json"
export HOMEBREW_NO_AUTO_UPDATE=1 HOMEBREW_NO_INSTALL_CLEANUP=1
python3 - "$project" <<'PY'
from pathlib import Path
import hashlib,json,sys
root=Path(sys.argv[1]);stage=root/'build/surface-diag-v725'
audit=json.loads((root/'reproducibility/source-lock.json').read_text())
c=json.loads((stage/'candidate.json').read_text())
for path,wanted in audit['audited_xnu_files'].items():
    actual=hashlib.sha256((root/'build/dxb/xnu'/path).read_bytes()).hexdigest()
    assert actual==(c['after_sha256'] if path==c['source'] else wanted),path
import time
saved=stage/('prior-build-'+time.strftime('%Y%m%d-%H%M%S'));saved.mkdir()
for name in ('build.log','build.rc','kernel.v725.macho'):
    file=stage/name
    if file.exists():file.rename(saved/name)
folder=root/'build/dxb/build/xnu.obj/DEVELOPMENT_ARM64_SWEET/iokit/DEVELOPMENT'
for name in ('SweetSurface.cpo','SweetSurface.cpd'):
    file=folder/name
    if file.exists():
        file.rename(saved/name)
kernel=root/'build/dxb/build/xnu.sym/kernel.development.sweet'
import shutil
shutil.copy2(kernel,saved/'kernel.reproduced-v724.macho')
link=root/'build/dxb/build/xnu.obj/kernel.development.sweet'
stripped=link.resolve()
assert stripped.is_relative_to(root/'build/dxb/build') and stripped.is_file()
link.rename(saved/'kernel.development.sweet.link')
stripped.rename(saved/'kernel.development.sweet.stripped')
print('78 source audit passed; prior objects and kernel retained')
PY
cd "$project/build/dxb"
export KERNEL_CONFIG=DEVELOPMENT ARCH_CONFIG=ARM64 MACHINE_CONFIG=SWEET MACOS_VERSION=13.5
export INCREMENTAL=1
set +e
bash build.sh > "$stage/build.log" 2>&1
diag_rc=$?
printf '%s\n' "$diag_rc" > "$stage/build.rc"
if [ "$diag_rc" -eq 0 ]; then
python3 - "$project" <<'PY'
from pathlib import Path
import hashlib,json,sys
root=Path(sys.argv[1]);stage=root/'build/surface-diag-v725'
kernel=root/'build/dxb/build/xnu.sym/kernel.development.sweet'
raw=kernel.read_bytes()
assert b'SWEET SURF CALLER DIAG phase=' in raw
assert 'SweetSurface.cpp' in (stage/'build.log').read_text(errors='replace')
import shutil
shutil.copy2(kernel,stage/'kernel.v725.macho')
print(json.dumps({'build_rc':0,'kernel_sha256':hashlib.sha256(raw).hexdigest(),'diagnostic_marker_linked':True}))
PY
diag_rc=$?
fi
exit "$diag_rc"
