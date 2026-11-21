"""Exercise production stencil bit-plane/sample transfers on host Mesa only."""
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
    files=['tools/build/test_stencil_sample_copy_gles.c','tools/build/check_stencil_sample_copy_gles.py',
           'gpu/sweetgl/StencilSampleCopyGLES.h','gpu/sweetgl/Stencil8ClearGLES.h','gpu/sweetgl/Depth32ClearGLES.h','gpu/sweetgl/DepthMultisampleCopyGLES.h']
    sources={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    command=['wsl.exe','-d',a.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/files[0]),'-lEGL','-lGLESv2','-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').write_bytes(result.stdout+result.stderr)
    assert result.returncode==0,(result.stdout+result.stderr)[-3000:].decode(errors='replace')
    raw=bounded_linux_test(a.distribution,out,'test',['EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    match=re.search(rb'Stencil sample copy PASS copies=(\d+) checked_uint_words=(\d+) depth_uint_words=(\d+) native_clears=(\d+) masked_draws=(\d+) bitpatterns=all256 samples=1,2,4 outer_pixels_and_CPU_guards=1 host_only=1',raw);assert match
    assert sources=={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    report=dict(source_files=sources,copies=int(match[1]),checked_uint_words=int(match[2]),depth_uint_words=int(match[3]),native_clears=int(match[4]),masked_draws=int(match[5]),
                UBSan_passed=True,host_renderer=re.search(rb'^Host renderer: (.+)$',raw,re.M)[1].decode(),actual_production_stencil_Load_Store_and_native_Clear=True,
                variable_all_256_byte_patterns_and_logical_1_2_4_samples_verified=True,depth_bits_and_unselected_stencil_samples_preserved=True,
                host_actual_texture_samples={m[1].decode():int(m[2]) for m in re.finditer(rb'test samples=(\d+) actual_samples=(\d+)',raw)},
                masked_author_rectangle_uses_fragment_discard_and_GL_scissor=True,empty_fragment_host_scissor_outlier_NOT_driver_acceptance=True,
                combined_and_destination_outer_pixels_and_CPU_read_guards_preserved=True,host_only_NOT_FD540_original_page_or_full_native_proof=True,device_operations=0)
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
