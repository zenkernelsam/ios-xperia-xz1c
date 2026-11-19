"""Check extracted production Clear shaders on real host Mesa; no phone access."""
import argparse, hashlib, json, subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu-22.04');a=p.parse_args()
out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
source=(ROOT/'gpu/sweetgl/metal_render.c').read_text()
def function(name):
    start=source.index('static int '+name+'(');brace=source.index('{',start);depth=1;at=brace+1;quoted=False;escaped=False
    while depth:
        c=source[at]
        if quoted:
            if escaped:escaped=False
            elif c=='\\':escaped=True
            elif c=='"':quoted=False
        elif c=='"':quoted=True
        elif c=='{':depth+=1
        elif c=='}':depth-=1
        at+=1
    return source[start:at]
production=(function('bind_xr_lut')+'\n'+function('bounded_clear')+'\n').encode()
internals=(ROOT/'gpu/sweetgl/MetalShaderInternals.h').read_text()
cache=internals[internals.index('typedef struct SweetMetalShaderCache'):internals.index('struct SweetMetalProgram')]
extracted=(cache+'\n').encode()+production
(out/'ProductionClear.h').write_bytes(extracted)
def linux(path):
    path=path.resolve();assert path.drive=='D:';return '/mnt/d/'+path.as_posix()[3:]
def run(argv,name):
    r=subprocess.run(list(map(str,argv)),capture_output=True);(out/(name+'.log')).write_bytes(r.stdout+r.stderr)
    assert r.returncode==0,(r.stdout+r.stderr).decode('utf-8','replace');return r.stdout
run(['wsl.exe','-d',a.distribution,'--','gcc','-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-I',linux(out),linux(ROOT/'tools/build/test_clear_cache.c'),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test-clear-cache')],'compile')
test=run(['wsl.exe','-d',a.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1',linux(out/'test-clear-cache')],'test')
assert b'production Clear cache PASS' in test
paths=['gpu/sweetgl/metal_render.c','gpu/sweetgl/MetalShaderInternals.h','gpu/sweetgl/XRColourShader.h','gpu/sweetgl/XRColourTables.h','tools/build/test_clear_cache.c','tools/build/check_clear_cache.py']
report=dict(source_files={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths},production_extracted_sha256=hashlib.sha256(extracted).hexdigest(),
            actual_GLES_Clear_cached_uncached_21_cases_bytes_equal=True,actual_context_isolation_failure_rollback_and_retirement=True,
            UBSan_passed=True,ASan_not_run=True,host_Mesa_only=True,FD540_padding_fences_and_original_UI_still_require_device=True,device_operations=0)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
