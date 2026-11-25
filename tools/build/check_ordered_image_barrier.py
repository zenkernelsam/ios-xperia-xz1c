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
PRIOR_HEADER_COMMIT="5aeb5417ada81aff446679fcbeb0cc1a6a0bcbda"
PRIOR_HEADER_SHA="f34cdc163da1b6270919f834c9e698d16f03c15d225780aa0b66103951e9451b"
REFERENCE_FILES={
    "src/gallium/drivers/freedreno/a5xx/fd5_emit.h":"a73e222a1e19e6449fdce37a0fea1d09c7ff09f60c0ce02a9826221c8dbf7b5d",
    "src/gallium/drivers/freedreno/a5xx/fd5_context.h":"d5c3d03f38dddd6a856b25f1cb2403694dee265ce4396f6c7e82864c559d8bd1",
    "src/gallium/drivers/freedreno/freedreno_context.c":"78f981df0565365f1b889482158e41ffbf0d29c196e44fdd2b16f0e19f890710",
    "src/mesa/state_tracker/st_atom_image.c":"3d5016983fd427ea2b56ef74f9cadee8e780873c37ddd95a252357c9b16c8755",
    "src/gallium/drivers/freedreno/freedreno_state.c":"ff7d4f47707c82692c3d60d7cbc3c014abb3e3ada0c51bad92c6bb9a4c2f2ad3",
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
           "tools/build/test_ordered_image_mapping.c",
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
    mapping_command=["clang","-std=gnu11","-O1","-g","-fsanitize=address,undefined",
                     "-fno-sanitize-recover=all","-ffunction-sections","-fdata-sections",
                     "-Wl,-dead_strip",*defines,*includes,
                     '-DSWEET_MESA_IMAGE_SOURCE="'+str(mesa/"src/mesa/state_tracker/st_atom_image.c")+'"',
                     str(ROOT/"tools/build/test_ordered_image_mapping.c"),"-o",str(output/"test_mapping")]
    run(output,mapping_command,"mapping-compile")
    mapping_text=run(output,[output/"test_mapping"],"mapping-test")
    if "Ordered image mapping PASS:" not in mapping_text:
        raise ValueError("Actual Mesa dense image mapping test completion missing")
    prior_bytes=subprocess.check_output(["git","-C",str(ROOT),"show",
                                        PRIOR_HEADER_COMMIT+":gpu/sweetgl/OrderedImageBarrier.h"])
    if hashlib.sha256(prior_bytes).hexdigest()!=PRIOR_HEADER_SHA:
        raise ValueError("Prior sparse image-mask source identity differs")
    prior=output/"PriorOrderedImageBarrier.h"
    with prior.open("xb") as stream:
        stream.write(prior_bytes)
    prior_command=["clang","-std=gnu11","-O1","-g","-fsanitize=address,undefined",
                   "-fno-sanitize-recover=all",*defines,*includes,
                   '-DSWEET_ORDERED_BARRIER_HEADER="'+str(prior)+'"',
                   "-DSWEET_EXPECT_DENSE_REJECTION=1",
                   str(ROOT/"tools/build/test_ordered_image_barrier.c"),"-o",str(output/"test_prior")]
    run(output,prior_command,"prior-compile")
    prior_text=run(output,[output/"test_prior"],"prior-test")
    if "actual dense two-slot layout refused by the old sparse mask check" not in prior_text:
        raise ValueError("Prior dense-image fallback regression was not reproduced")
    if source_files!={path:sha(ROOT/path) for path in files}:
        raise ValueError("Barrier sources changed during verification")
    report=dict(revision=4,source_files=source_files,Mesa_commit=MESA_COMMIT,
                actual_Mesa_sources=REFERENCE_FILES,
                commands=[list(map(str,command)),[str(output/"test_barrier")],
                          list(map(str,mapping_command)),[str(output/"test_mapping")],
                          list(map(str,prior_command)),[str(output/"test_prior")]],
                host_ASan_UBSan_actual_emitters_passed=True,first_barrier_ring_words=26,subsequent_barrier_ring_words=21,
                first_timestamp_scratch_GPU_initialization_verified=True,
                monotonic_timestamp_and_exact_wait_verified=True,sequence_overflow_refused=True,
                star_draw_host_primitives=1404,star_draw_host_ring_words=29489,
                authored_batch_changes_preserve_context_sequence=11,
                authored_continuous_ring_guarded_growths=9,
                actual_Mesa_image_unit_to_dense_driver_slot_mapping_passed=True,
                image_masks_checked=255,image_declaration_orders_checked=2,
                unknown_or_duplicate_bindings_access_atomic_and_SSBO_writes_refused=True,
                prior_sparse_mask_false_fallback_reproduced=True,
                prior_sparse_header_commit=PRIOR_HEADER_COMMIT,prior_sparse_header_sha256=PRIOR_HEADER_SHA,
                actual_Mesa_scheduler_or_GPU_execution_verified=False,
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
