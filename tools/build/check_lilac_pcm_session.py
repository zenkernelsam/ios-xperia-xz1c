"""Reconstruct locked APR headers and verify the offline AFE/ADM/ASM session."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys

from check_lilac_afe_slimbus import materialize_headers, run

ROOT = Path(__file__).resolve().parents[2]
TESTS = (
    ("pcm", "test_lilac_asm_pcm.c"),
    ("queue", "test_lilac_asm_pcm_queue.c"),
    ("apr", "test_lilac_asm_pcm_apr.cpp"),
    ("map", "test_lilac_asm_pcm_map.cpp"),
    ("stream", "test_lilac_asm_pcm_stream.cpp"),
    ("afe", "test_lilac_afe_slimbus_playback.cpp"),
    ("session", "test_lilac_pcm_playback_session.cpp"),
)
SUPPORT = (
    "LilacAsmPcm.h", "LilacAsmPcmQueue.h", "LilacAsmPcmApr.h",
    "LilacAsmPcmMap.h", "LilacAsmPcmStream.h",
    "LilacAdmPlaybackDevice.h", "LilacAdmPlaybackRoute.h",
    "LilacAfeSlimbusPlayback.h", "LilacPcmPlaybackSession.h",
)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def check(output, arm64_object=False):
    output = Path(output).resolve()
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output under formal build/")
    if os.name != "nt" and sys.platform != "darwin":
        raise ValueError("Use the formal Windows/WSL or Mac host")
    if arm64_object and sys.platform != "darwin":
        raise ValueError("Compile the iOS ARM64 object with the formal Mac SDK")
    source_paths = ["xnu/support/" + name for name in SUPPORT]
    source_paths += ["tools/build/" + name for _, name in TESTS]
    source_paths += ["tools/build/check_lilac_pcm_session.py", "tools/build/check_lilac_afe_slimbus.py",
                     "tools/build/compile_lilac_pcm_session.cpp"]
    source_files = {path: sha(ROOT / path) for path in source_paths}
    output.mkdir(parents=True)
    headers = materialize_headers(output)

    def local_path(path):
        path = Path(path).resolve()
        if os.name == "nt":
            return "/mnt/" + path.drive[0].lower() + "/" + path.as_posix()[3:]
        return str(path)

    prefix = ["wsl.exe", "-d", "Ubuntu-22.04", "--"] if os.name == "nt" else []
    commands, outputs = [], {}
    for name, filename in TESTS:
        cpp = filename.endswith(".cpp")
        command = prefix + ["clang++" if cpp else "clang", "-std=c++17" if cpp else "-std=c11",
                            "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]
        if not cpp:
            command += ["-pedantic"]
        if os.name == "nt":
            command += ["-no-pie"]
        binary = output / ("test_" + name)
        command += ["-I", local_path(output), local_path(ROOT / "tools/build" / filename),
                    "-o", local_path(binary)]
        execution = prefix + [local_path(binary)]
        run(output, command, name + "-compile")
        text = run(output, execution, name + "-test")
        if name == "session" and "Lilac PCM session PASS:" not in text:
            raise ValueError("Actual session test did not complete")
        commands.extend([command, execution])
        outputs[name] = {
            "binary_sha256": sha(binary),
            "test_log_sha256": sha(output / (name + "-test.log")),
            "ASan_UBSan_passed": True,
        }
    target_object = None
    if arm64_object:
        sdk_command = ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"]
        sdk = run(output, sdk_command, "arm64-sdk").strip()
        if not Path(sdk).is_dir():
            raise ValueError("iOS SDK directory not found")
        binary = output / "pcm-session.arm64.o"
        command = ["xcrun", "--sdk", "iphoneos", "clang++", "-target", "arm64-apple-ios16.7",
                   "-isysroot", sdk, "-std=c++17", "-O1", "-Wall", "-Wextra", "-Werror",
                   "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-I", str(output), "-c",
                   str(ROOT / "tools/build/compile_lilac_pcm_session.cpp"), "-o", str(binary)]
        run(output, command, "arm64-compile")
        file_command, symbols_command = ["file", str(binary)], ["nm", "-u", str(binary)]
        description = run(output, file_command, "arm64-file")
        undefined = run(output, symbols_command, "arm64-undefined")
        if "Mach-O 64-bit object arm64" not in description:
            raise ValueError("Expected an ARM64 Mach-O object")
        commands.extend([sdk_command, command, file_command, symbols_command])
        target_object = dict(sha256=sha(binary), description=description.strip(),
                             undefined_symbols=undefined.splitlines(), linked_into_kernel=False)
    if source_files != {path: sha(ROOT / path) for path in source_paths}:
        raise ValueError("Playback sources changed during verification")
    report = dict(
        revision=1, source_files={**headers, **source_files}, commands=commands, tests=outputs,
        arm64_object=target_object,
        host_ASan_UBSan_passed=True,
        playback_format={"channels": 2, "bits_per_sample": 16, "sample_rate": 48000},
        pending_buffer_capacity=4,
        startup_actions=["OpenStream", "SetStreamFormat", "MapRegion", "ConfigureAfe",
                         "StartAfe", "OpenOutput", "RouteOutput", "Run"],
        shutdown_actions=["all WRITE_DONE", "EndOfStream basic reply and RENDERED_EOS",
                          "CloseStream", "UnmapRegion", "CloseOutput", "StopAfe"],
        input_consumption_and_rendered_EOS_checked_separately=True,
        basic_EOS_reply_and_rendered_event_either_order=True,
        readiness_bound_to_exact_epoch=True,
        late_success_does_not_release_uncertain_region=True,
        all_13_command_failure_boundaries_and_pending_write_error_checked=True,
        exclusive_serialized_transport_owner_required=True,
        codec_channels_schema_and_firmware_token_echo_require_hardware_validation=True,
        transport_connected=False, linked_into_device_kernel=False,
        DSP_codec_or_physical_audio_verified=False, device_operations=0,
    )
    with (output / "manifest.json").open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True)
    parser.add_argument("--arm64-object", action="store_true")
    args = parser.parse_args()
    print(json.dumps(check(args.output, args.arm64_object)))
