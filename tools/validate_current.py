"""Validate canonical source/manifest/script integrity without writing bytecode or operating a phone."""
import hashlib,json,os,re,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
lock=json.loads((root/'reproducibility/source-lock.json').read_text(encoding='utf-8'))
assert hashlib.sha256((root/'xnu/lilac-current.patch').read_bytes()).hexdigest()==lock['xnu']['patch_sha256']
assert hashlib.sha256((root/'gpu/mesa-lilac.patch').read_bytes()).hexdigest()==lock['mesa']['patch_sha256']
assert hashlib.sha256((root/'tools/build/dxb-local.patch').read_bytes()).hexdigest()==lock['builder']['patch_sha256']
for version,name in ((725,'lilac-surface-diagnostics.patch'),(726,'lilac-surface-memref-diagnostics.patch'),(727,'lilac-surface-readonly.patch'),(728,'lilac-surface-region64.patch')):
    candidate=json.loads((root/f'reproducibility/surface-diag-v{version}.json').read_text())
    assert candidate['version']==version
    assert hashlib.sha256((root/'xnu'/name).read_bytes()).hexdigest()==candidate['patch_sha256']
for path,digest in lock['sweetgl']['files'].items():
    file=root/'gpu/sweetgl'/path
    if path in ('mesa-setup.sh','mesa.patch'):
        assert not file.exists();continue
    wanted=lock['sweetgl'].get('build_script_updated_for_canonical_output') if path=='build.sh' else digest
    assert hashlib.sha256(file.read_bytes()).hexdigest()==wanted,path
devices=json.loads((root/'reproducibility/current-device-tools.json').read_text(encoding='utf-8'))
for path,digest in devices['sources'].items():
    assert hashlib.sha256((root/'tools/device'/path).read_bytes()).hexdigest()==digest,path
for path,digest in devices.get('shared_sources',{}).items():
    assert path in ('tools/diagnostic_candidate.py','tools/mkpayload.py','tools/ram_library.py')
    assert hashlib.sha256((root/path).read_bytes()).hexdigest()==digest,path
py_files=[]
for base in ('tools','gpu'):
    for file in (root/base).rglob('*.py'):
        if any(part in ('__pycache__','out') for part in file.parts):continue
        compile(file.read_bytes(),str(file),'exec');py_files.append(file)
patch=(root/'xnu/lilac-current.patch').read_bytes()
for row in re.findall(rb'^diff --git (.+)$',patch,re.M):
    assert not re.search(rb'(\.orig|\.rej|\.DS_Store|\.bak\S*)$',row),row
for path in ('AGENTS.md','README.md','docs/STATUS.md','docs/BUILD.md','docs/LAYOUT.md','docs/OPERATIONS.md','docs/ROOT-GL5.md'):
    assert (root/path).is_file()
for path in ('tools/build/build_kernel.sh','tools/build/build_geocompat.sh','gpu/sweetgl/build.sh','tools/device/guarded_xperable.sh'):
    assert b'\r' not in (root/path).read_bytes(),path
if os.name!='nt':
    for file in [root/'tools/build/build_kernel.sh',root/'tools/build/build_geocompat.sh',root/'gpu/sweetgl/build.sh',root/'tools/device/guarded_xperable.sh']:
        subprocess.run(['bash','-n',str(file)],check=True)
print(json.dumps({'canonical_xnu_patch_paths':lock['xnu']['current_patch_paths'],'audited_xnu_sources':len(lock['audited_xnu_files']),
    'python_scripts_syntax_checked':len(py_files),'canonical_sources_match_current_lock':True,
    'runtime_acceptance_requires_actual_evidence':True,'source_only_validation':True,'device_operations':0}))
