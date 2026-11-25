"""Compile actual pinned Mesa emitters and check the authored XR barrier ring."""

import argparse
import hashlib
import json
from pathlib import Path
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
MESA_COMMIT="769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de"
REFERENCE_FILES={
    "src/gallium/drivers/freedreno/a5xx/fd5_emit.h":"a73e222a1e19e6449fdce37a0fea1d09c7ff09f60c0ce02a9826221c8dbf7b5d",
    "src/gallium/drivers/freedreno/a5xx/fd5_context.h":"d5c3d03f38dddd6a856b25f1cb2403694dee265ce4396f6c7e82864c559d8bd1",
    "src/gallium/drivers/freedreno/freedreno_context.c":"78f981df0565365f1b889482158e41ffbf0d29c196e44fdd2b16f0e19f890710",
}


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def run(output,command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,timeout=60)
    with (output/(name+".log")).open("xb") as stream:
        stream.write(result.stdout+result.stderr)
    if result.returncode:
        raise ValueError(name+" failed: "+result.stderr.decode("utf-8","replace")[-2000:])
    return result.stdout.decode("utf-8","replace")


def check(mesa_build,output):
    output=Path(output).resolve()
    mesa_build=Path(mesa_build).resolve()
    mesa=ROOT/"deps/mesa"
    if sys.platform!="darwin":
        raise ValueError("Run this command-stream host test on the formal Mac build host")
    if not output.is_relative_to(ROOT/"build") or output.exists() or not mesa_build.is_relative_to(ROOT/"build"):
        raise ValueError("Use existing formal Mesa inputs and a new build output")
    if subprocess.check_output(["git","-C",str(mesa),"rev-parse","HEAD"]).decode().strip()!=MESA_COMMIT:
        raise ValueError("Mesa revision differs from the pinned A5xx emitters")
    for path,digest in REFERENCE_FILES.items():
        if sha(mesa/path)!=digest:
            raise ValueError("Actual pinned Mesa source differs: "+path)
    entries=json.loads((mesa_build/"compile_commands.json").read_bytes())
    selected=[entry for entry in entries if entry["file"].endswith("state_tracker/st_manager.c")]
    if len(selected)!=1:
        raise ValueError("Need one actual state-tracker compilation command")
    defines=[word for word in shlex.split(selected[0]["command"]) if word.startswith("-D")]
    # Keep host assertions and use the host's endian declarations. The
    # production iOS frontend retains its original target definitions.
    defines=[word for word in defines if word not in ("-DNDEBUG","-DHAVE_ENDIAN_H")]
    include_sources=("include","src","src/mesa","src/mapi","src/gallium/include","src/gallium/auxiliary",
                     "src/gallium/drivers","src/gallium/winsys","src/freedreno","src/freedreno/common",
                     "src/freedreno/drm","src/gallium/drivers/freedreno","src/gallium/drivers/freedreno/ir3",
                     "include/drm-uapi")
    include_generated=("src","src/mesa","src/mapi","src/freedreno/registers/adreno","src/compiler","src/compiler/nir")
    includes=["-I"+str(mesa/path) for path in include_sources]
    includes+=["-I"+str(mesa_build/path) for path in include_generated]
    includes+=["-I"+str(ROOT/"gpu/sweetgl/include")]
    files=("gpu/sweetgl/OrderedImageBarrier.h","tools/build/test_ordered_image_barrier.c",
           "tools/build/check_ordered_image_barrier.py")
    source_files={path:sha(ROOT/path) for path in files}
    output.mkdir(parents=True)
    command=["clang","-std=gnu11","-O1","-g","-fsanitize=address,undefined",
             "-fno-sanitize-recover=all",*defines,*includes,
             str(ROOT/"tools/build/test_ordered_image_barrier.c"),"-o",str(output/"test_barrier")]
    run(output,command,"compile")
    text=run(output,[output/"test_barrier"],"test")
    if "Ordered image barrier PASS:" not in text:
        raise ValueError("Actual command-stream test completion missing")
    if source_files!={path:sha(ROOT/path) for path in files}:
        raise ValueError("Barrier sources changed during verification")
    report=dict(revision=2,source_files=source_files,Mesa_commit=MESA_COMMIT,
                actual_Mesa_sources=REFERENCE_FILES,commands=[list(map(str,command)),[str(output/"test_barrier")]],
                host_ASan_UBSan_actual_emitters_passed=True,ring_words_per_barrier=21,
                monotonic_timestamp_and_exact_wait_verified=True,sequence_overflow_refused=True,
                context_flushes=0,host_command_stream_only=True,device_operations=0)
    with (output/"manifest.json").open("x",encoding="utf-8",newline="\n") as stream:
        json.dump(report,stream,indent=2)
        stream.write("\n")
    return report


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mesa-build",default=str(ROOT/"build/mesa-ir3-cache-r1"))
    parser.add_argument("--output",required=True)
    args=parser.parse_args()
    print(json.dumps(check(args.mesa_build,args.output)))
