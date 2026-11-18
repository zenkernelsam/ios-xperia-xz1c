"""Install the source-checked v730 display delta, retaining the exact predecessor."""
import json
import subprocess
import sys
import time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT, audit, configuration, sha


def main():
    config = configuration(730)
    manifest = config['manifest']
    tree = ROOT / 'build/dxb/xnu'
    host = json.loads((ROOT / 'build/scanout-snapshot-v730/host-checks/manifest.json').read_bytes())
    assert host['ASan_UBSan_passed'] and host['ARM64_compile_passed'] and host['wire_fault_tests_passed']
    for name in ('SweetFramebufferScreen.h', 'SweetFramebufferScreenRuntime.h'):
        assert sha((ROOT / 'xnu/support' / name).read_bytes()) == host['source_files']['xnu/support/' + name]
        assert config['source_hashes']['iokit/Kernel/arm/' + name] == host['source_files']['xnu/support/' + name]
    audit(729)
    backup = ROOT / 'build/scanout-snapshot-v730' / ('installed-from-' + time.strftime('%Y%m%d-%H%M%S'))
    backup.mkdir()
    for path, digest in manifest['base_sources'].items():
        file = tree / path
        raw = file.read_bytes()
        assert sha(raw) in (digest, manifest['prior_actual_sources'][path])
        saved = backup / path
        saved.parent.mkdir(parents=True, exist_ok=True)
        saved.write_bytes(raw)
        normalized = raw.replace(b'\r\n', b'\n')
        assert sha(normalized) == digest
        file.write_bytes(normalized)
    patch = ROOT / 'xnu/lilac-scanout-snapshot.patch'
    subprocess.run(['git', 'apply', '--check', patch], cwd=tree, check=True)
    subprocess.run(['git', 'apply', patch], cwd=tree, check=True)
    stage = ROOT / 'build/surface-diag-v730'
    stage.mkdir(exist_ok=True)
    result = audit(730)
    (stage / 'installation.json').write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print(json.dumps(result, indent=2))


if __name__ == '__main__': main()
