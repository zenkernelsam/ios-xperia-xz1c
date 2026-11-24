"""Canonical versioned diagnostic source/kernel bindings; no device operations."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MEMORY_SOURCE = 'iokit/Kernel/IOMemoryDescriptor.cpp'
MEMORY_BASE = '9963ef1a82cfe5e1bfca797c17c686c54eeea6e631ed349babfc80ac8c824379'
PATCHES = {725: 'lilac-surface-diagnostics.patch', 726: 'lilac-surface-memref-diagnostics.patch',727:'lilac-surface-readonly.patch',728:'lilac-surface-region64.patch',729:'lilac-console-snapshot.patch',730:'lilac-scanout-snapshot.patch'}
WLED_VERSION = 731
WLED_BUILD = 'build/lilac-wled-isolated-link-r3-20261124'
WLED_KERNEL_SHA = '15fd8eee111bc880b9b54aeee284f9dc91cd2b7dbbbe302dc944b00d8d2d3e69'
WLED_LINK_PROOF_SHA = '933c7d0bd9ece16e86f26992a8df13cc95c9e91d0c72fdf6fb8e0a8409a319ad'
WLED_SOURCES = {
    'iokit/Kernel/arm/SweetFramebufferScreenRuntime.h': '2c566a962d61e7b8aebfc88be54d496878821a4c0f5bb439f505198b16adfba2',
    'iokit/Kernel/arm/LilacWledSnapshot.h': '77c51e06bd55d6752c215f36a4e5346840ea631d662fdf199b9f03a82799fd0d',
    'iokit/Kernel/arm/LilacWledScreenProbe.h': '35c4ae4e9815daa479e8f429d991700d72528fe67405d8ca4710fb176c510ef0',
}
WLED_MARKER = b'SWEET WLED SAMPLE token='


def sha(data):
    return hashlib.sha256(data).hexdigest()


def configuration(version):
    if type(version) is int and version == WLED_VERSION:
        base = configuration(730)
        raw = (ROOT / 'reproducibility/lilac-wled-isolated-link-20261124.json').read_bytes()
        manifest = json.loads(raw)
        assert manifest['candidate_kernel_sha256'] == WLED_KERNEL_SHA
        assert manifest['link_proof_sha256'] == WLED_LINK_PROOF_SHA
        binding = {**base['binding'], 'version': version, 'manifest_sha256': sha(raw),
                   'base_manifest_sha256': base['binding']['manifest_sha256'],
                   'link_proof_sha256': WLED_LINK_PROOF_SHA}
        return {'manifest': manifest, 'binding': binding, 'kernel_sha256': WLED_KERNEL_SHA,
                'source_hashes': {**base['source_hashes'], **WLED_SOURCES},
                'changed_sources': list(WLED_SOURCES), 'stage': ROOT / WLED_BUILD}
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
    if version>=729:
        fifth=json.loads((ROOT/'reproducibility/surface-diag-v729.json').read_bytes())
        assert fifth['base_version']==728 and fifth['logging_only'] and fifth['GPU_allocator_SCM_PON_RPC_VM_code_unchanged']
        assert set(fifth['base_sources'])==set(fifth['changed_sources'])=={'bsd/dev/vermeer_debug.c','bsd/dev/vermeer_log_ring.h'}
        assert all(fifth['overrides'][p]==h for p,h in fourth['overrides'].items())
        expected.update(fifth['overrides']);changed=fifth['changed_sources']
    if version>=730:
        source='iokit/Kernel/arm/SweetFramebuffer.cpp'
        headers={source,'iokit/Kernel/arm/SweetFramebufferScreen.h','iokit/Kernel/arm/SweetFramebufferScreenRuntime.h'}
        assert manifest['base_version']==729 and manifest['read_only_scanout_snapshot'] and manifest['GPU_allocator_SCM_PON_RPC_VM_code_unchanged']
        assert set(manifest['base_sources'])=={source} and set(manifest['changed_sources'])==headers
        assert manifest['prior_actual_sources'][source]==baseline['audited_xnu_files'][source]
        assert all(manifest['overrides'][p]==h for p,h in fifth['overrides'].items())
        assert set(manifest['overrides'])==set(fifth['overrides'])|headers
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
    if record['version'] == WLED_VERSION:
        assert record['kernel_sha256'] == config['kernel_sha256'], 'WLED kernel identity differs'
        assert record['source_revision'] == config['binding']['manifest_sha256'], 'WLED source revision differs'
    return config


def kernel_path(version):
    config = configuration(version)
    name = 'kernel.wled-observer.macho' if version == WLED_VERSION else f'kernel.v{version}.macho'
    return config['stage'] / name


def verify_wled_kernel(version, raw):
    if version == WLED_VERSION:
        assert sha(raw) == WLED_KERNEL_SHA and WLED_MARKER in raw, 'Expected the exact linked WLED candidate'
    elif WLED_MARKER in raw:
        raise ValueError('The WLED candidate requires its own diagnostic version')


def audit(version, kernel_sha=None):
    config = configuration(version)
    if version == WLED_VERSION:
        audit(730, config['manifest']['base_kernel_sha256'])
        stage = config['stage']
        receipt = (stage / 'link-proof.json').read_bytes()
        assert sha(receipt) == WLED_LINK_PROOF_SHA
        proof = json.loads(receipt)
        assert proof['overlay_sources_sha256'] == WLED_SOURCES
        assert proof['candidate_kernel_sha256'] == WLED_KERNEL_SHA
        assert sha((stage / 'SweetFramebuffer.cpo').read_bytes()) == proof['candidate_framebuffer_object_sha256']
        assert sha((stage / 'SweetFramebuffer.cpp').read_bytes()) == config['source_hashes']['iokit/Kernel/arm/SweetFramebuffer.cpp']
        assert all(sha((stage / Path(path).name).read_bytes()) == digest for path, digest in WLED_SOURCES.items())
        verify_wled_kernel(version, kernel_path(version).read_bytes())
        assert kernel_sha is None or kernel_sha == WLED_KERNEL_SHA, 'Compiled WLED kernel differs from trial'
        return {'version': version, 'audited_sources': len(config['source_hashes']),
                'source_binding': config['binding'], 'actual_sources_match': True,
                'kernel_sha256': WLED_KERNEL_SHA, 'diagnostic_markers_linked': True}
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
        if version>=730:assert b'SWEET SCREEN BEGIN token=' in raw and b'SWEET SCREEN SCAN token=' in raw
        result['kernel_sha256'] = sha(raw)
        result['diagnostic_markers_linked'] = True
    return result
