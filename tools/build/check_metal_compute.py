"""Exercise actual AIR and Metal compute bodies on the host; no GPU proof."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from materialize_air_sources import root,materialize
parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--llvm-headers',required=True)
args=parser.parse_args();out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
materialize(out/'sources');headers=Path(args.llvm_headers).resolve();common=['-std=c++17','-DSWEET_LLVM_PRE23=1','-DSWEET_LLVM16_FORCE_FALLBACK=1','-I',out/'sources','-I',headers]
library='/System/Library/PrivateFrameworks/GPUCompiler.framework/Versions/32023/Libraries/libLLVM.dylib'
link=['-Wl,-undefined,dynamic_lookup','-Wl,-needed_library,'+library]
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True);(out/(name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode==0,str(out/(name+'.log'))+'\n'+(result.stdout+result.stderr)[-4000:];return result.stdout
run(['clang++',*common,'-g','-O1','-fsanitize=address,undefined',root/'tools/build/test_llvm16_compat.cpp',*link,'-o',out/'compat-test'],'compat-compile')
assert 'PASS' in run([out/'compat-test'],'compat-test')
object_common=[argument for argument in common]
# The actual Metal source resolves its retained headers by their real path;
# use the same directory for its angle includes instead of a second copy.
object_common[object_common.index(out/'sources')]=root/'gpu/metal/air/reference'
run(['clang++',*common,'-g','-O1','-fsanitize=address,undefined','-c',out/'sources/Frontend.cpp',
    '-o',out/'frontend-test.o'],'frontend-compile')
run(['clang++',*object_common,'-g','-O1','-fsanitize=address,undefined','-fno-objc-arc','-c',
    root/'tools/build/test_metal_compute.mm','-o',out/'compute-test.o'],'compute-compile')
run(['clang++','-fsanitize=address,undefined',out/'frontend-test.o',out/'compute-test.o',*link,
    '-framework','Metal','-framework','Foundation','-o',out/'compute-test'],'compute-link')
assert 'PASS' in run([out/'compute-test',root/'gpu/metal/air/fixtures/basic.air.ll'],'compute-test')
run(['clang++',*object_common,'-g','-O1','-fsanitize=address,undefined','-fno-objc-arc','-c',
    root/'tools/build/test_metal_render.mm','-o',out/'render-test.o'],'render-compile')
run(['clang++','-fsanitize=address,undefined',out/'frontend-test.o',out/'render-test.o',*link,
    '-framework','Metal','-framework','Foundation','-o',out/'render-test'],'render-link')
assert 'PASS' in run([out/'render-test',root/'gpu/metal/air/fixtures/render.air.ll'],'render-test')
run(['clang++',*object_common,'-g','-O1','-fsanitize=address,undefined','-fno-objc-arc','-c',
    root/'tools/build/test_metal_constants.mm','-o',out/'constants-test.o'],'constants-compile')
run(['clang++','-fsanitize=address,undefined',out/'frontend-test.o',out/'constants-test.o',*link,
    '-framework','Metal','-framework','Foundation','-o',out/'constants-test'],'constants-link')
assert 'PASS' in run([out/'constants-test',root/'gpu/metal/air/fixtures/constants.air.ll'],'constants-test')
paths=['gpu/metal/MetalCompute.inc','gpu/metal/MetalCommandQueue.inc','gpu/metal/MetalResources.mm',
    'gpu/metal/MetalRender.inc','gpu/metal/MetalFunctionConstants.inc','gpu/metal/MetalTextureRules.h','gpu/metal/air/ESSLTarget.h','gpu/metal/air/LLVM16Compat.h',
    'gpu/metal/air/fixtures/render.air.ll','gpu/metal/air/fixtures/constants.air.ll','tools/build/test_metal_constants.mm',
    'tools/build/test_metal_render.mm','tools/build/test_metal_compute.mm','tools/build/test_llvm16_compat.cpp']
report=dict(source_files={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in paths},
    real_legacy_LLVM_adapter_ASan_UBSan_passed=True,real_Metal_compute_objects_ASan_UBSan_passed=True,
    real_AIR_to_ESSL_translation_passed=True,host_GPU_stub_not_hardware_proof=True,device_operations=0)
report.update(real_Metal_texture_render_objects_ASan_UBSan_passed=True,real_AIR_graphics_translation_passed=True)
report['real_native_Metal_function_constants_ASan_UBSan_passed']=True
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
