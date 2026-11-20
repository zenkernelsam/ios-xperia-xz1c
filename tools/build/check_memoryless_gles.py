"""Exercise authored transient-pass draw recipes on real host Mesa only."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from check_depth_stencil_gles import ROOT, linux, bounded_linux_test

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True);parser.add_argument('--distribution',default='Ubuntu')
    args=parser.parse_args();out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['tools/build/test_memoryless_gles.c','tools/build/check_memoryless_gles.py','tools/build/check_depth_stencil_gles.py',
           'gpu/sweetgl/DepthStencilGLES.h','gpu/sweetgl/MetalDepthStencilRules.h','gpu/sweetgl/Depth32ClearGLES.h','gpu/sweetgl/Stencil8ClearGLES.h']
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    command=['wsl.exe','-d',args.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/paths[0]),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test')]
    p=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').open('xb').write(p.stdout+p.stderr);assert p.returncode==0,(p.stdout+p.stderr)[-4000:].decode(errors='replace')
    data=bounded_linux_test(args.distribution,out,'test',['EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    matched=re.search(rb'Memoryless GLES recipes PASS: phases=8 draws=(\d+) checked_bytes=(\d+) exact_depth_stencil_pixels=1 independent_stores=1 host_only=1',data);assert matched
    assert sources=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    proof=dict(source_files=sources,host_renderer=data.decode().splitlines()[0],phases=8,draws=int(matched[1]),checked_bytes=int(matched[2]),UBSan_passed=True,
               original_depth_stencil_readback_and_separate_store_recipe_verified=True,host_native_objects_and_FD540_original_page_import_not_executed=True,
               actual_Memoryless_device_acceptance_pending=True,host_time_limit_seconds=45,host_log_limit_bytes=1048576,device_operations=0)
    (out/'manifest.json').open('xb').write((json.dumps(proof,indent=2)+'\n').encode());print(json.dumps(proof))
if __name__=='__main__':main()
