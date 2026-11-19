"""Exercise the shared depth/stencil state on real host Mesa, without a phone."""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def linux(path):
    path=Path(path).resolve();assert path.drive=='D:'
    return '/mnt/d/'+path.as_posix()[3:]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--distribution',default='Ubuntu')
    args=parser.parse_args();out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['gpu/sweetgl/MetalDepthStencilRules.h','tools/build/test_depth_stencil_gles.c','tools/build/check_depth_stencil_gles.py','gpu/sweetgl/DepthStencilGLES.h','gpu/sweetgl/Depth32ClearGLES.h']
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    command=['wsl.exe','-d',args.distribution,'--','gcc','-g','-O1','-fsanitize=address,undefined',linux(ROOT/paths[1]),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True);(out/'compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr).decode(errors='replace')
    result=subprocess.run(['wsl.exe','-d',args.distribution,'--','env','EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0',linux(out/'test')],capture_output=True)
    (out/'test.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr).decode(errors='replace')
    text=result.stdout.decode('utf-8');matched=re.search(r'Depth/stencil GLES PASS: draws=(\d+) checked_pixels=(\d+) all_compare_ops=8 all_stencil_ops=8 front_back=1 scissor_masks_guards=1 host_only=1',text);assert matched
    assert sources=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    report=dict(source_files=sources,host_renderer=text.splitlines()[0],draws=int(matched[1]),checked_pixels=int(matched[2]),
                actual_shared_GL_state_compares_and_stencil_ops_passed=True,ASan_UBSan_passed=True,
                host_leak_detection_disabled_for_Mesa=True,host_attachment_format='GL_DEPTH32F_STENCIL8',
                candidate_not_yet_connected_to_native_Metal_or_FD540_importer=True,device_operations=0,
                original_GPU_wallpaper_and_peripheral_goal_not_achieved=True)
    (out/'manifest.json').open('xb').write((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))


if __name__=='__main__':main()
