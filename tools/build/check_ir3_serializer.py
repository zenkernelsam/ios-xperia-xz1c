"""Native sanitizer check using actual pinned Mesa types and serializer helpers."""
import argparse,hashlib,json,shlex,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--mesa-build',default='build/mesa-program-cache-r2');a=p.parse_args()
out=Path(a.output).absolute();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
build=Path(a.mesa_build).absolute();commands=json.loads((build/'compile_commands.json').read_bytes())
entry=next(c for c in commands if c['file'].endswith('ir3_disk_cache.c'))
flags=[v for v in shlex.split(entry['command']) if (v.startswith('-I') or v.startswith('-D')) and v not in ('-DNDEBUG','-DHAVE_ENDIAN_H','-DHAVE_XLOCALE_H')]
flags+=['-I'+str(ROOT/'gpu/mesa')]
def run(command,name):
    result=subprocess.run(list(map(str,command)),cwd=build,capture_output=True);(out/(name+'.log')).write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr).decode('utf-8','replace');return result.stdout
source=ROOT/'tools/build/test_ir3_serializer.c';binary=out/'test-serializer'
run(['clang','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-Wl,-dead_strip',*flags,source,
     ROOT/'deps/mesa/src/util/blob.c',ROOT/'deps/mesa/src/util/ralloc.c','-o',binary],'compile')
assert b'real IR3 serializer PASS' in run([binary],'test')
paths=['gpu/mesa/IR3VariantCache.h','tools/build/test_ir3_serializer.c','tools/build/check_ir3_serializer.py']
report=dict(source_files={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},actual_Mesa_types_blob_ralloc=True,
    exact_draw_and_binning_payload_roundtrip=True,partial_restore_not_published=True,bounds_and_pointer_rebasing_passed=True,
    ASan_UBSan_passed=True,host_only=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
