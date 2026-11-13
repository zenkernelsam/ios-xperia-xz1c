"""Exercise real bridge/object bodies without a phone; compile the iOS frontend."""
import argparse,errno,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);args=parser.parse_args()
out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
paths=['gpu/sweetgl/sweet_drm.c','gpu/sweetgl/MetalBackingAPI.h','gpu/sweetgl/MetalBackingRules.h',
       'gpu/metal/MetalResources.h','gpu/metal/MetalResources.mm','gpu/metal/MetalBufferRules.h','gpu/metal/MetalDeviceLimits16.h',
       'gpu/sweetgl/MetalBlitPlan.h','gpu/metal/MetalCommandQueue.inc','gpu/metal/MetalQueueState.h',
       'tools/build/test_metal_backing.c','tools/build/test_metal_resources.mm','tools/build/test_metal_queue.mm','tools/build/test_metal_blit_plan.c','tools/build/test_metal_blit_bridge.c']
source_files={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths}
for path in paths:
    target=out/'sources'/path;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes((root/path).read_bytes())
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    return result.stdout
mesa=root/'deps/mesa'
run(['clang','-g','-O1','-fsanitize=address,undefined','-I',root/'gpu/sweetgl/include','-I',mesa/'include','-I',mesa/'include/drm-uapi',
     root/'tools/build/test_metal_backing.c','-framework','IOKit','-framework','CoreFoundation','-o',out/'test_bridge'],'bridge-compile')
assert 'PASS' in run([out/'test_bridge'],'bridge-test')
run(['clang','-g','-O1','-fsanitize=address,undefined','-I',root/'gpu/sweetgl/include','-I',mesa/'include','-I',mesa/'include/drm-uapi',
    root/'tools/build/test_metal_blit_bridge.c','-framework','IOKit','-framework','CoreFoundation','-o',out/'test_blit_bridge'],'blit-bridge-compile')
blit=run([out/'test_blit_bridge'],'blit-bridge-test');assert 'PASS' in blit
assert blit.count('SWEET METAL BLIT submit call=')==5 and blit.count('native_return=1')==4
assert 'result='+str(-errno.ETIMEDOUT) in blit and 'result='+str(-errno.ENODEV) in blit
run(['clang++','-std=c++17','-DSWEET_METAL_HOST_TEST=1','-DSWEET_METAL_NO_COMPUTE=1','-g','-O1','-fsanitize=address,undefined','-fno-objc-arc',
    root/'tools/build/test_metal_resources.mm','-framework','Foundation','-framework','Metal','-o',out/'test_resources'],'objects-compile')
assert 'PASS' in run([out/'test_resources'],'objects-test')
run(['clang','-g','-O1','-fsanitize=address,undefined',root/'tools/build/test_metal_blit_plan.c','-o',out/'test_plan'],'plan-compile')
assert 'PASS' in run([out/'test_plan'],'plan-test')
run(['clang++','-std=c++17','-DSWEET_METAL_HOST_TEST=1','-DSWEET_METAL_NO_COMPUTE=1','-g','-O1','-fsanitize=address,undefined','-fno-objc-arc',
    root/'tools/build/test_metal_queue.mm','-framework','Foundation','-framework','Metal','-o',out/'test_queue'],'queue-compile')
assert 'PASS' in run([out/'test_queue'],'queue-test')
run(['xcrun','--sdk','iphoneos','clang++','-arch','arm64','-miphoneos-version-min=16.0','-std=c++17','-DSWEET_METAL_NO_COMPUTE=1','-fno-objc-arc',
    '-c',root/'gpu/metal/MetalResources.mm','-o',out/'MetalResources.o'],'ios-compile')
assert all(hashlib.sha256((root/p).read_bytes()).hexdigest()==h for p,h in source_files.items())
report=dict(source_files=source_files,actual_bridge_bodies_host_asan_ubsan_passed=True,actual_Metal_object_bodies_host_asan_ubsan_passed=True,
    ios_arm64_compilation_passed=True,real_device_resource_and_rendering_acceptance_pending=True,device_operations=0)
report.update(actual_queue_objects_host_asan_ubsan_passed=True,actual_A5xx_packet_builder_host_asan_ubsan_passed=True)
report['actual_blit_transport_host_asan_ubsan_passed']=True
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
