"""Build a separate observer and exercise the original exception chain natively."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/'gpu/diagnostics/exception_observe.m';before=source.read_bytes()
(out/'source.m').write_bytes(before)
def run(argv,name):
    result=subprocess.run(list(map(str,argv)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    return result.stdout
run(['clang','-DLILAC_EXCEPTION_TEST=1','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',
     source,'-framework','Foundation','-o',out/'test_observer'],'host-compile')
test=run([out/'test_observer'],'host-test')
assert test.count('SWEET EXCEPTION begin call=')==16 and 'PASS' in test
assert 'reason="line1 line2"' in test
assert test.count('SWEET METAL uniform call=')==32 and 'renderer ABI, nil/device metadata' in test
assert 'device_nil=1 device_class=nil new_buffer_method=0' in test
assert 'device_nil=0 device_class=NUNITestDevice new_buffer_method=1' in test
library=out/'exception_observe.dylib'
run(['xcrun','--sdk','iphoneos','clang','-arch','arm64','-miphoneos-version-min=16.0',
    '-fobjc-exceptions','-O2','-dynamiclib',source,'-framework','Foundation',
    '-Wl,-not_for_dyld_shared_cache','-install_name','/dev/sglram/e','-o',library],'ios-compile')
run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'verify-signature')
assert source.read_bytes()==before
manifest=dict(source='gpu/diagnostics/exception_observe.m',source_sha256=hashlib.sha256(before).hexdigest(),
    library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),library_bytes=library.stat().st_size,
    native_original_chain_and_bound_test_passed=True,host_asan_ubsan_passed=True,
    original_exception_behavior_preserved=True,target_process='AegirPoster',observations_max=16,device_operations=0)
manifest.update(renderer_abi_and_device_observer_test_passed=True,renderer_observations_max=32,
    metadata_only_no_buffer_allocation=True,original_renderer_return_and_exception_preserved=True)
(out/'manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
