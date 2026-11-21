"""Compile actual production boxes and the original fixed Mesa dimension gate."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

ROOT=Path(__file__).resolve().parents[2]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--mesa-build',default='build/mesa-ir3-cache-r1');a=p.parse_args()
    out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    mesa=ROOT/'deps/mesa';build=Path(a.mesa_build).resolve();assert build.is_relative_to(ROOT/'build')
    assert subprocess.check_output(['git','-C',str(mesa),'rev-parse','HEAD'],text=True).strip()=='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de'
    sources=['gpu/sweetgl/DepthTiledCopyBox.h','tools/build/test_depth_tiled_copy_box.c','tools/build/check_depth_tiled_copy_box.py']
    original=['src/util/box.h','src/gallium/include/pipe/p_state.h','src/gallium/drivers/freedreno/a5xx/fd5_blitter.c']
    before={f:sha(ROOT/f) for f in sources};pinned={f:sha(mesa/f) for f in original}
    src=(mesa/original[-1]).read_text();match=re.search(r'static bool\nok_dims\([^\n]*\n\{.*?\n\}',src,re.S);assert match
    (out/'PinnedFD5Dimensions.h').write_text(match[0]+'\n')
    compile_entries=json.loads((build/'compile_commands.json').read_text())
    entry=next(x for x in compile_entries if x['file'].endswith('state_tracker/st_manager.c'))
    defines=[x for x in shlex.split(entry['command']) if x.startswith('-D') and x!='-DNDEBUG']
    includes=[ROOT/'gpu/sweetgl',out,mesa/'include',mesa/'src',mesa/'src/gallium/include',mesa/'src/gallium/auxiliary',build/'src',build/'src/mesa',build/'src/mapi']
    command=['xcrun','clang','-O1','-g','-fsanitize=address,undefined','-std=c11',*defines]
    for directory in includes:command+=['-I',str(directory)]
    command += [str(ROOT/sources[1]),'-o',str(out/'test')]
    result=subprocess.run(command,capture_output=True,text=True,timeout=45);(out/'compile.log').write_text(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-3000:]
    result=subprocess.run([str(out/'test')],capture_output=True,text=True,timeout=15);(out/'test.log').write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
    assert 'old=(0,0,9,0,17,1) fixed=(0,0,0,17,9,1)' in result.stdout
    assert before=={f:sha(ROOT/f) for f in sources} and pinned=={f:sha(mesa/f) for f in original}
    report=dict(source_files=before,pinned_Mesa_sources=pinned,actual_unmodified_fd5_ok_dims_source_sha256=hashlib.sha256(match[0].encode()).hexdigest(),
                actual_pinned_pipe_box_header_used=True,old_positional_initializer_rejected=True,production_designated_box_admitted=True,
                powers_of_two_and_one_past_dimensions_and_z_refused=True,ASan_UBSan_passed=True,device_operations=0,host_only_not_GPU_copy_evidence=True,
                result=result.stdout.strip())
    (out/'manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
