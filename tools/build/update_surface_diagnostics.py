"""Apply the complete v725 diagnostic delta to the pinned reconstructed baseline."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[2]
stage = root / 'build/surface-diag-v725'
tree = root / 'build/dxb/xnu'
manifest = json.loads((root / 'reproducibility/surface-diag-v725.json').read_text())
lock = json.loads((root / 'reproducibility/source-lock.json').read_text())
delta = root / 'xnu/lilac-surface-diagnostics.patch'
source = tree / manifest['source']

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

assert sha(delta) == manifest['patch_sha256']
assert lock['audited_xnu_files'][manifest['source']] == manifest['before_sha256']
for path, wanted in lock['audited_xnu_files'].items():
    if path == manifest['source']:
        assert sha(tree / path) in (manifest['before_sha256'], manifest['after_sha256'])
    else:
        assert sha(tree / path) == wanted, path
stage.mkdir(exist_ok=True)
saved = stage / 'SweetSurface.v724.cpp'
if sha(source) == manifest['before_sha256']:
    if saved.exists():
        assert sha(saved) == manifest['before_sha256']
    else:
        shutil.copy2(source, saved)
    subprocess.run(['git', 'apply', '--check', str(delta)], cwd=tree, check=True)
    subprocess.run(['git', 'apply', str(delta)], cwd=tree, check=True)
assert sha(source) == manifest['after_sha256']
(stage / 'candidate.json').write_bytes((json.dumps(manifest, indent=2) + '\n').encode())
print(json.dumps({'audited_sources': 78, 'diagnostic_delta_applied': True,
                  'source_sha256': sha(source), 'device_operations': 0}))