"""Record authoritative source/build evidence without publishing raw build logs."""
import hashlib,json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
lock=json.loads((root/'reproducibility/source-lock.json').read_text(encoding='utf-8'))
bad=[]
for path,wanted in lock['audited_xnu_files'].items():
    p=root/'build/dxb/xnu'/path
    if hashlib.sha256(p.read_bytes()).hexdigest()!=wanted:bad.append(path)
assert not bad,bad
kernel=root/'build/dxb/build/xnu.sym/kernel.development.sweet'
gles=root/'build/sweetgl/sweetgl.dylib'
klog=root/'build/kernel-reproduce-pinned.log';glog=root/'build/gles-reproduce-link-targets.log'
assert kernel.exists() and gles.exists()
assert b'XNU Build Done!' in klog.read_bytes() and b'CANONICAL_GLES_BUILD_COMPLETE' in glog.read_bytes()
for p in (kernel,gles):
    assert p.read_bytes()[:4]==b'\xcf\xfa\xed\xfe'
def version(argv):return subprocess.check_output(argv,text=True).strip()
record={'reconstruction_audited_sources':len(lock['audited_xnu_files']),'audited_sources_match_live_v724':True,
        'kernel_compilation_complete':True,'gles_compilation_complete':True,
        'kernel_sha256':hashlib.sha256(kernel.read_bytes()).hexdigest(),'kernel_bytes':kernel.stat().st_size,
        'gles_sha256':hashlib.sha256(gles.read_bytes()).hexdigest(),'gles_bytes':gles.stat().st_size,
        'kernel_build_log_sha256':hashlib.sha256(klog.read_bytes()).hexdigest(),
        'gles_build_log_sha256':hashlib.sha256(glog.read_bytes()).hexdigest(),
        'new_binaries_device_tested':False,
        'device_tested_kernel':'da6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a',
        'device_tested_gles':'9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571',
        'xcode':version(['xcodebuild','-version']),'sdk':version(['xcrun','--sdk','iphoneos','--show-sdk-version']),
        'bison':version(['/usr/local/opt/bison/bin/bison','--version']).splitlines()[0],
        'flex':version(['/usr/local/opt/flex/bin/flex','--version']).splitlines()[0],
        'build_outputs_not_for_automatic_flash':True}
(root/'reproducibility/build-results.json').write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
print(json.dumps(record,indent=2))
