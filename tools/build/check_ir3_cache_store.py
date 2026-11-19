"""Run the actual bounded IR3 persistence layer without hardware access."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
header=b'static const unsigned char sweet_program_cache_build_id[32]={0x19,0x83,0x54};\n'
(out/'ProgramCacheBuildIdentity.h').write_bytes(header)
files=out/'files';files.mkdir()
def linux(path):return '/mnt/'+path.drive[0].lower()+path.as_posix()[2:]
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr).decode('utf-8','replace');return result.stdout
run(['wsl.exe','-d','Ubuntu-22.04','--','gcc','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-I',linux(out),linux(ROOT/'tools/build/test_ir3_cache_store.c'),'-lcrypto','-lpthread','-o',linux(out/'test-ir3-store')],'compile')
assert b'IR3 record/file PASS' in run(['wsl.exe','-d','Ubuntu-22.04','--',linux(out/'test-ir3-store'),linux(files)],'test')
paths=['gpu/sweetgl/IR3CacheAPI.h','gpu/sweetgl/IR3CacheRecord.h','gpu/sweetgl/ir3_cache.c','tools/build/test_ir3_cache_store.c','tools/build/check_ir3_cache_store.py']
report=dict(source_files={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},fixture_build_ID_only=True,
    exact_actual_persistence_and_fresh_process_passed=True,invalid_bytes_rejected_without_overwriting=True,
    UBSan_passed=True,ASan_not_run=True,device_operations=0,actual_IR3_serializer_and_hardware_proof_still_required=True)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
