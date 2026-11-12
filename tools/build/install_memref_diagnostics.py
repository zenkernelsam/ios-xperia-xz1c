"""Install the already-sanitized v726 delta with fixed before/after source hashes."""
import json
import shutil
import subprocess
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT, MEMORY_SOURCE, MEMORY_BASE, audit, configuration, sha

config = configuration(726); stage = config['stage']; stage.mkdir(exist_ok=True)
source = ROOT / 'build/dxb/xnu' / MEMORY_SOURCE
current = sha(source.read_bytes())
if current == MEMORY_BASE:
    audit(725)
    saved = stage / 'IOMemoryDescriptor.before.cpp'
    if saved.exists(): assert sha(saved.read_bytes()) == MEMORY_BASE
    else: shutil.copy2(source, saved)
    patch = ROOT / 'xnu/lilac-surface-memref-diagnostics.patch'
    subprocess.run(['git', 'apply', '--check', str(patch)], cwd=ROOT / 'build/dxb/xnu', check=True)
    subprocess.run(['git', 'apply', str(patch)], cwd=ROOT / 'build/dxb/xnu', check=True)
else:
    assert current == config['manifest']['after_sha256'], 'Unrecognized IOMemoryDescriptor edits'
proof = audit(726)
report = stage / 'installation.json'
if report.exists():
    assert json.loads(report.read_bytes()) == proof
else:
    report.write_bytes((json.dumps(proof, indent=2) + '\n').encode())
print(json.dumps(proof, indent=2))
