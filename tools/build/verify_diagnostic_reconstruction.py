"""Rebuild diagnostic sources from the pinned OSS base in a fresh TEMP checkout.

No current build tree, device or original source is modified; outputs are kept.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tempfile
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT, PATCHES, configuration, MEMORY_SOURCE, MEMORY_BASE


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', type=int, choices=sorted(PATCHES), default=730)
    parser.add_argument('--source-cache', default=str(ROOT / 'build/dxb/xnu'))
    args = parser.parse_args()
    lock = json.loads((ROOT / 'reproducibility/source-lock.json').read_bytes())
    config = configuration(args.version)
    out = Path(tempfile.mkdtemp(prefix=f'lilac-xnu-v{args.version}-reconstruction-'))
    tree = out / 'xnu'
    events = []

    def run(argv, name, cwd=None):
        result = subprocess.run(list(map(str, argv)), cwd=cwd, capture_output=True)
        with (out / (name + '.log')).open('xb') as file:
            file.write(result.stdout + result.stderr)
        assert result.returncode == 0, (result.stdout + result.stderr).decode('utf-8', 'replace')
        return result.stdout

    cache = Path(args.source_cache).resolve()
    run(['git', 'clone', '--shared', '--no-checkout', cache, tree], 'clone')
    run(['git', 'checkout', '--detach', lock['xnu']['base_commit']], 'checkout', tree)
    full = ROOT / 'xnu/lilac-current.patch'
    assert digest(full.read_bytes()) == lock['xnu']['patch_sha256']
    run(['git', 'apply', '--check', full], 'base-check', tree)
    run(['git', 'apply', full], 'base-apply', tree)
    expected = dict(lock['audited_xnu_files'])
    expected[MEMORY_SOURCE] = MEMORY_BASE
    assert all(digest((tree / path).read_bytes()) == sha for path, sha in expected.items())
    for version, name in sorted(PATCHES.items()):
        if version > args.version: break
        patch = ROOT / 'xnu' / name
        raw = patch.read_bytes()
        manifest = json.loads((ROOT / f'reproducibility/surface-diag-v{version}.json').read_bytes())
        assert digest(raw) == manifest['patch_sha256']
        normalized = []
        for line in raw.splitlines():
            if not line.startswith(b'--- a/'): continue
            path = line[6:].decode('utf-8').split('\t', 1)[0]
            relative = PurePosixPath(path)
            assert not relative.is_absolute() and '..' not in relative.parts
            file = tree / path
            before = file.read_bytes()
            after = before.replace(b'\r\n', b'\n')
            if before != after:
                file.write_bytes(after)
                normalized.append(dict(path=path, before_sha256=digest(before), after_sha256=digest(after)))
        run(['git', 'apply', '--check', patch], f'v{version}-check', tree)
        run(['git', 'apply', patch], f'v{version}-apply', tree)
        wanted = configuration(version)['source_hashes']
        assert all(digest((tree / path).read_bytes()) == sha for path, sha in wanted.items()), f'v{version} reconstruction differs'
        events.append(dict(version=version, patch_sha256=digest(raw), audited_sources=len(wanted),
            actual_inputs_match=True, normalized_predecessors=normalized))
    report = dict(version=args.version, source_binding=config['binding'],
        OSS_base_commit=lock['xnu']['base_commit'], full_patch_sha256=digest(full.read_bytes()),
        fresh_apply_passed=True, source_hashes=config['source_hashes'], audited_sources=len(config['source_hashes']),
        stages=events, work_directory=str(out), outputs_preserved=True, original_build_tree_unchanged=True,
        device_operations=0, binary_or_hardware_pass_not_inferred=True)
    with (out / 'report.json').open('xb') as file:
        file.write((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
