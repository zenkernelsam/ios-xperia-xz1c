"""Canonical versioned diagnostic source/kernel bindings; no device operations."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MEMORY_SOURCE = 'iokit/Kernel/IOMemoryDescriptor.cpp'
MEMORY_BASE = '9963ef1a82cfe5e1bfca797c17c686c54eeea6e631ed349babfc80ac8c824379'
PATCHES = {725: 'lilac-surface-diagnostics.patch', 726: 'lilac-surface-memref-diagnostics.patch',727:'lilac-surface-readonly.patch',728:'lilac-surface-region64.patch',729:'lilac-console-snapshot.patch'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def configuration(version):
    assert type(version) is int and version in PATCHES, 'Unsupported diagnostic version'
    path = ROOT / f'reproducibility/surface-diag-v{version}.json'
    raw = path.read_bytes(); manifest = json.loads(raw)
    assert manifest['version'] == version
    patch_hashes = {}
    for number in sorted(PATCHES):
        if number > version: continue
        m = json.loads((ROOT / f'reproducibility/surface-diag-v{number}.json').read_bytes())
        name = PATCHES[number]
        digest = sha((ROOT / 'xnu' / name).read_bytes())
        assert digest == m['patch_sha256'], name
        patch_hashes[name] = digest
    baseline = json.loads((ROOT / 'reproducibility/source-lock.json').read_bytes())
    first = json.loads((ROOT / 'reproducibility/surface-diag-v725.json').read_bytes())
    assert first['before_sha256'] == baseline['audited_xnu_files'][first['source']]
    expected = dict(baseline['audited_xnu_files'])
    expected[first['source']] = first['after_sha256']
    expected[MEMORY_SOURCE] = MEMORY_BASE
    changed = [first['source']]
    if version >= 726:
        second=json.loads((ROOT/'reproducibility/surface-diag-v726.json').read_bytes())
        manifest_726=second
        assert manifest_726['source'] == MEMORY_SOURCE and manifest_726['before_sha256'] == MEMORY_BASE
        expected.update(manifest_726['overrides']); changed.append(MEMORY_SOURCE)
    if version == 726:
        assert manifest['source'] == MEMORY_SOURCE and manifest['before_sha256'] == MEMORY_BASE
        assert manifest['overrides'][first['source']] == first['after_sha256']
        assert manifest['overrides'][MEMORY_SOURCE] == manifest['after_sha256']
        assert set(manifest['overrides']) == {first['source'], MEMORY_SOURCE}
        expected.update(manifest['overrides'])
    if version >= 727:
        third=json.loads((ROOT/'reproducibility/surface-diag-v727.json').read_bytes())
        assert third['base_version']==726
        assert third['overrides'][MEMORY_SOURCE]==second['after_sha256']
        assert set(third['changed_sources'])==set(third['overrides'])-{MEMORY_SOURCE}
        expected.update(third['overrides'])
        changed=sorted(set(changed+third['changed_sources']))
    if version>=728:
        fourth=json.loads((ROOT/'reproducibility/surface-diag-v728.json').read_bytes())
        assert fourth['base_version']==727 and fourth['before_sha256']==third['overrides'][fourth['source']]
        assert fourth['overrides'][fourth['source']]==fourth['after_sha256']
        assert all(h==third['overrides'][p] for p,h in fourth['overrides'].items() if p!=fourth['source'])
        expected.update(fourth['overrides'])
    if version==729:
        assert manifest['base_version']==728 and manifest['logging_only'] and manifest['GPU_allocator_SCM_PON_RPC_VM_code_unchanged']
        assert set(manifest['base_sources'])==set(manifest['changed_sources'])=={'bsd/dev/vermeer_debug.c','bsd/dev/vermeer_log_ring.h'}
        assert all(manifest['overrides'][p]==h for p,h in fourth['overrides'].items())
        expected.update(manifest['overrides']);changed=manifest['changed_sources']
    binding = {'version': version, 'manifest_sha256': sha(raw), 'patches': patch_hashes}
    return {'manifest': manifest, 'binding': binding, 'source_hashes': expected,
            'changed_sources': changed, 'stage': ROOT / f'build/surface-diag-v{version}'}


def verify_record(record):
    config = configuration(record['version'])
    # Original v725 results predate this additional binding; their image and raw
    # evidence remain independently checked. New versions require the binding.
    if record['version'] >= 726 or 'diagnostic_binding' in record:
        assert record['diagnostic_binding'] == config['binding'], 'Diagnostic source binding differs'
    return config


def audit(version, kernel_sha=None):
    config = configuration(version)
    tree = ROOT / 'build/dxb/xnu'
    for path, digest in config['source_hashes'].items():
        assert sha((tree / path).read_bytes()) == digest, path
    result = {'version': version, 'audited_sources': len(config['source_hashes']),
              'source_binding': config['binding'], 'actual_sources_match': True}
    if kernel_sha:
        kernel = config['stage'] / f'kernel.v{version}.macho'
        raw = kernel.read_bytes()
        assert sha(raw) == kernel_sha, 'Compiled kernel differs from trial'
        assert b'SWEET SURF CALLER DIAG phase=' in raw
        if version >= 726:
            assert b'SWEET SURF MEMREF ERROR address=' in raw and b'SWEET SURF MEMREF INIT length=' in raw
        if version>=727:assert b'SWEET SURF READONLY id=' in raw
        result['kernel_sha256'] = sha(raw)
        result['diagnostic_markers_linked'] = True
    return result
