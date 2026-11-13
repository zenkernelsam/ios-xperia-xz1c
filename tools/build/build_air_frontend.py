"""Compile the same OSS AIR reader for host and iOS16 using verified C imports."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path
from materialize_air_sources import materialize,root
parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--llvm-headers',required=True)
parser.add_argument('--target-symbols',required=True);args=parser.parse_args()
out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
materialize(out/'sources');headers=Path(args.llvm_headers).resolve()
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    return result.stdout
common=['-std=c++17','-DSWEET_LLVM_PRE23=1','-I',out/'sources','-I',headers]
source=out/'sources/Frontend.cpp';cli=root/'tools/build/air_frontend_cli.cpp'
host_library='/System/Library/PrivateFrameworks/GPUCompiler.framework/Versions/32023/Libraries/libLLVM.dylib'
run(['clang++',*common,'-O1','-g','-fsanitize=address,undefined',source,cli,
    '-Wl,-undefined,dynamic_lookup','-Wl,-needed_library,'+host_library,'-framework','Foundation','-o',out/'air-cli'],'host-compile')
run(['clang++',*common,'-DSWEET_LLVM16_FORCE_FALLBACK=1','-O1','-g','-fsanitize=address,undefined',source,cli,
    '-Wl,-undefined,dynamic_lookup','-Wl,-needed_library,'+host_library,'-framework','Foundation','-o',out/'air-cli-forced'],'forced-host-compile')
run(['xcrun','--sdk','iphoneos','clang++','-arch','arm64','-miphoneos-version-min=16.0',*common,
    '-O2','-c',source,'-o',out/'AIRFrontend.ios.o'],'ios-compile')
run(['python3',root/'tools/build/audit_air_llvm.py','--object',out/'AIRFrontend.ios.o','--target-symbols',args.target_symbols,
    '--output',out/'llvm-import-audit.json'],'target-import-audit')
audit=json.loads((out/'llvm-import-audit.json').read_bytes());assert not audit['missing_target_imports']
fixture=root/'gpu/metal/air/fixtures/basic.air.ll'
actual=json.loads(run([out/'air-cli',fixture],'basic-test'));assert len(actual)==3
forced=json.loads(run([out/'air-cli-forced',fixture],'forced-basic-test'));assert forced==actual
assert [(e['name'],e['stage']) for e in actual]==[('add_seven',0),('position',1),('color',2)]
assert actual[0]['write_mask']==actual[0]['buffer_mask']==1
assert actual[1]['buffer_mask']==1 and actual[1]['write_mask']==0
paths=[cli,root/'gpu/metal/air/Compiler.h',root/'gpu/metal/air/LLVM16Compat.h',root/'gpu/metal/air/AIRFrontend.cpp',fixture]
paths += [root/row['destination'] for row in json.loads((root/'reproducibility/air-reference.json').read_bytes())['sources'].values()]
report=dict(source_files={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
    host_asan_ubsan_fixture_passed=True,ios_arm64_compilation_passed=True,llvm_import_audit=audit,
    forced_legacy_interface_host_test_passed=True,forced_and_available_C_entry_translation_equal=True,
    compiled_AIR_object_sha256=hashlib.sha256((out/'AIRFrontend.ios.o').read_bytes()).hexdigest(),
    actual_AIR_stage_translation_verified=True,GPU_execution_and_runtime_target_still_pending=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps({k:v for k,v in report.items() if k!='llvm_import_audit'}))
