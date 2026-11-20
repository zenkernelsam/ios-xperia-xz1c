"""Exercise the shared depth/stencil state on real host Mesa, without a phone."""
import argparse
import hashlib
import json
import re
import shlex
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]


def linux(path):
    path=Path(path).resolve();assert path.drive=='D:'
    return '/mnt/d/'+path.as_posix()[3:]


def bounded_linux_test(distribution,out,name,environment,executable):
    # Both process time and output bytes are bounded inside the WSL process.
    # prlimit applies before the test starts, so recursive sanitizer output
    # cannot allocate an unbounded Python buffer or error message on Windows.
    log=out/(name+'.log')
    assert not log.exists()
    child=['env',*environment,linux(executable)]
    # WSL's forwarded stdout is a pipe, which is not subject to RLIMIT_FSIZE.
    # Redirect inside Linux before exec so the limit covers the actual file.
    script='exec '+shlex.join(child)+' > '+shlex.quote(linux(log))+' 2>&1'
    # The limit also covers the shell so its termination message cannot append
    # bytes after a child reaches the cap.
    result=subprocess.run(['wsl.exe','-d',distribution,'--','timeout','--signal=KILL','45','prlimit','--fsize=1048576','--','sh','-c',script],capture_output=True,timeout=55)
    data=log.read_bytes();assert len(data)<=1048576
    assert result.returncode==0,(data+result.stdout+result.stderr)[-4000:].decode(errors='replace')
    return data


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--output',required=True);parser.add_argument('--distribution',default='Ubuntu')
    parser.add_argument('--sanitizers',choices=('address,undefined','undefined'),default='address,undefined')
    args=parser.parse_args();out=Path(args.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    paths=['gpu/sweetgl/MetalDepthStencilRules.h','tools/build/test_depth_stencil_gles.c','tools/build/check_depth_stencil_gles.py','gpu/sweetgl/DepthStencilGLES.h','gpu/sweetgl/Depth32ClearGLES.h','gpu/sweetgl/Stencil8ClearGLES.h','gpu/sweetgl/StencilResourcePair.h','tools/build/test_stencil_resource_pair.c']
    sources={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    command=['wsl.exe','-d',args.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize='+args.sanitizers,linux(ROOT/paths[1]),'-lEGL','-lGLESv2','-lm','-o',linux(out/'test')]
    result=subprocess.run(command,capture_output=True,timeout=55);(out/'compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-4000:].decode(errors='replace')
    # Disable the runtime's recursive SIGSEGV handler, not its memory checks.
    # The Linux timeout terminates the exact host test even if its ASan startup
    # fails; never print an unbounded assertion message from captured output.
    data=bounded_linux_test(args.distribution,out,'test',['EGL_PLATFORM=surfaceless','LIBGL_ALWAYS_SOFTWARE=1','ASAN_OPTIONS=detect_leaks=0:handle_segv=0','UBSAN_OPTIONS=halt_on_error=1'],out/'test')
    text=data.decode('utf-8');matched=re.search(r'Depth/stencil GLES PASS: draws=(\d+) checked_pixels=(\d+) all_compare_ops=8 all_stencil_ops=8 front_back=1 scissor_masks_guards=1 host_only=1',text);assert matched
    result=subprocess.run(['wsl.exe','-d',args.distribution,'--','timeout','--signal=KILL','45','gcc','-g','-O1','-fsanitize='+args.sanitizers,linux(ROOT/'tools/build/test_stencil_resource_pair.c'),'-o',linux(out/'test-pair')],capture_output=True,timeout=55)
    (out/'pair-compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0,(result.stdout+result.stderr)[-4000:].decode(errors='replace')
    data=bounded_linux_test(args.distribution,out,'pair-test',['ASAN_OPTIONS=detect_leaks=0:handle_segv=0','UBSAN_OPTIONS=halt_on_error=1'],out/'test-pair')
    assert b'Stencil resource pair PASS:' in data
    assert sources=={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in paths}
    report=dict(source_files=sources,host_renderer=text.splitlines()[0],draws=int(matched[1]),checked_pixels=int(matched[2]),
                actual_shared_GL_state_compares_and_stencil_ops_passed=True,ASan_UBSan_passed=args.sanitizers=='address,undefined',
                selected_sanitizers=args.sanitizers,UBSan_passed=True,ASan_recursive_SIGSEGV_handler_disabled=True,
                host_test_timeout_seconds=45,host_test_log_byte_limit=1048576,
                original_pair_ownership_fault_rollback_host_model_passed=True,
                host_leak_detection_disabled_for_Mesa=True,host_attachment_format='GL_DEPTH32F_STENCIL8',
                host_test_does_not_execute_native_Metal_or_FD540_importer=True,device_operations=0,
                original_GPU_wallpaper_and_peripheral_goal_not_achieved=True)
    (out/'manifest.json').open('xb').write((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))


if __name__=='__main__':main()
