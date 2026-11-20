"""Run the real native author probe with an explicit non-hardware backend model."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tools/device'))
from multisample_evidence import verify

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--native-host',required=True);parser.add_argument('--fixture',required=True);parser.add_argument('--llvm-headers',required=True);args=parser.parse_args()
    out=Path(args.output).absolute();host=Path(args.native_host).resolve();fixture=Path(args.fixture).resolve();headers=Path(args.llvm_headers).resolve()
    assert out.is_relative_to(root/'build') and not out.exists() and host.is_relative_to(root/'build') and fixture.is_relative_to(root/'build');out.mkdir(parents=True)
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    manifest=json.loads((host/'manifest.json').read_bytes());assert all(sha(root/name)==digest for name,digest in manifest['source_files'].items())
    fm=json.loads((fixture/'manifest.json').read_bytes());assert fm['fixture']=='multisample' and sha(fixture/'AIRMultisampleFixture.h')==fm['header_sha256'] and sha(root/fm['source'])==fm['source_sha256']
    paths=['tools/build/test_metal_multisample_probe.mm','tools/build/check_metal_multisample_probe.py','gpu/diagnostics/MetalMultisampleProbe.h','gpu/diagnostics/MultisampleProbeValues.h','tools/device/multisample_evidence.py']
    sources={p:sha(root/p) for p in paths};sources.update(manifest['source_files'])
    library='/System/Library/PrivateFrameworks/GPUCompiler.framework/Versions/32023/Libraries/libLLVM.dylib'
    def run(cmd,name):
        result=subprocess.run(list(map(str,cmd)),capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-4000:].decode(errors='replace');return result.stdout
    run(['clang++','-std=c++17','-DSWEET_LLVM_PRE23=1','-DSWEET_LLVM16_FORCE_FALLBACK=1','-g','-O1','-fsanitize=address,undefined','-fno-objc-arc',
         '-I',root/'gpu/metal/air/reference','-I',headers,'-I',fixture,'-c',root/paths[0],'-o',out/'probe.o'],'compile')
    run(['clang++','-fsanitize=address,undefined',host/'frontend-test.o',out/'probe.o','-Wl,-undefined,dynamic_lookup','-Wl,-needed_library,'+library,
         '-framework','Foundation','-framework','Metal','-framework','IOSurface','-o',out/'probe'],'link')
    data=run([out/'probe'],'test');assert b'Actual author MSAA probe host model PASS:' in data
    token='0123456789abcdef';text=data.decode();result=verify('SWEET LOOP LIVE proto=1 token='+token+'\n'+text+'SWEET LOOP ACK FASTBOOT token='+token+'\n',token,fm)
    assert all(sha(root/name)==digest for name,digest in sources.items())
    report=dict(source_files=sources,actual_author_probe_native_objects_ASan_UBSan_passed=True,actual_probe_logs_match_independent_original_byte_page_resolve_recipe=True,phases=result['phases'],checked_bytes=result['checked_bytes'],guard_bytes=result['guard_bytes'],
                original_buffer_inputs_retained_per_draw=True,full_source_resolve_guard_and_final_retirement_host_model_passed=True,host_model_only_not_device_proof=True,device_operations=0)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
