"""Actual individual MSAA sample draws/clear/resolve on host Mesa; no phone."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
from check_depth_stencil_gles import ROOT,linux,bounded_linux_test
def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--distribution',default='Ubuntu');p.add_argument('--generated-shaders');a=p.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    files=['tools/build/test_multisample_gles.c','tools/build/check_multisample_gles.py','gpu/sweetgl/MultisampleLayout.h','gpu/sweetgl/MultisampleResolveGLES.h',
           'gpu/sweetgl/Depth32ClearGLES.h','gpu/sweetgl/Stencil8ClearGLES.h','gpu/sweetgl/DepthStencilGLES.h','gpu/sweetgl/MetalDepthStencilRules.h']
    sources={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    environment=['EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','UBSAN_OPTIONS=halt_on_error=1']
    generated={}
    if a.generated_shaders:
        directory=Path(a.generated_shaders).resolve();assert directory.is_relative_to(ROOT/'build')
        generated={name:hashlib.sha256((directory/name).read_bytes()).hexdigest() for name in ('multisample_position.vert','multisample_color.frag')}
        environment+=['SWEET_MSAA_GENERATED_DIR='+linux(directory)]
    command=['wsl.exe','-d',a.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize=undefined',linux(ROOT/files[0]),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').open('xb').write(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-3000:].decode(errors='replace')
    data=bounded_linux_test(a.distribution,out,'test',environment,out/'test')
    match=re.search(rb'Multisample GLES PASS: draws=(\d+) individual_samples=(\d+) checked_bytes=(\d+) resolves=(\d+) samples=2,4 depth_filters=sample0,min,max source_samples_unchanged=1 host_only=1',data);assert match
    assert sources=={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in files}
    if generated:assert generated=={name:hashlib.sha256((directory/name).read_bytes()).hexdigest() for name in generated}
    report=dict(source_files=sources,host_renderer=data.decode().splitlines()[0],draws=int(match[1]),individual_samples=int(match[2]),checked_bytes=int(match[3]),resolves=int(match[4]),
                samples=[2,4],original_color_depth_stencil_sample_masks_and_clear_passed=True,production_GPU_resolve_shader_colour_average_depth_sample0_min_max_passed=True,
                UBSan_passed=True,host_native_FD540_external_page_import_not_executed=True,actual_device_acceptance_pending=True,host_time_limit_seconds=45,host_log_limit_bytes=1048576,device_operations=0)
    if generated:report.update(actual_generated_AIR_vertex_fragment_MSAA_GLES_passed=True,generated_shader_sha256=generated)
    (out/'manifest.json').open('xb').write((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
if __name__=='__main__':main()
