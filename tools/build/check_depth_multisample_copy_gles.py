"""Exercise production sample shaders on host Mesa, without external page import."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from check_depth_stencil_gles import ROOT,linux,bounded_linux_test


def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu');a=p.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    files=['tools/build/test_depth_multisample_copy_gles.c','tools/build/check_depth_multisample_copy_gles.py','gpu/sweetgl/DepthMultisampleCopyGLES.h','gpu/sweetgl/Stencil8ClearGLES.h']
    sources={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    command=['wsl.exe','-d',a.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/files[0]),'-lEGL','-lGLESv2','-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-3000:].decode(errors='replace')
    raw=bounded_linux_test(a.distribution,out,'test',['EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    match=re.search(rb'Depth MSAA copy PASS copies=(\d+) checked_uint_words=(\d+) stencil_samples=(\d+) float_bits_exact=1 depth_and_stencil_preserved=1 destination_outer_pixels_and_CPU_guards=1 samples=2,4 host_only=1',raw);assert match
    assert sources=={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    report=dict(source_files=sources,copies=int(match[1]),checked_uint_words=int(match[2]),stencil_samples=int(match[3]),UBSan_passed=True,host_renderer=raw.decode().splitlines()[0],
        actual_depth_MSAA_Load_Store_float_bits_shader_semantics=True,combined_stencil_clear_preserves_depth_and_depth_load_preserves_stencil=True,
        destination_outer_pixels_and_CPU_read_guards_preserved=True,host_only_NOT_FD540_original_page_or_full_native_proof=True,device_operations=0)
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
