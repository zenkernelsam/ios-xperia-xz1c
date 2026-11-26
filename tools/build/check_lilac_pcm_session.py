"""Reconstruct locked APR headers and verify the offline AFE/ADM/ASM session."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys

from check_lilac_afe_slimbus import FIFO_HEADERS, materialize_headers, run

ROOT = Path(__file__).resolve().parents[2]
TESTS = (
    ("pcm", "test_lilac_asm_pcm.c"),
    ("queue", "test_lilac_asm_pcm_queue.c"),
    ("apr", "test_lilac_asm_pcm_apr.cpp"),
    ("map", "test_lilac_asm_pcm_map.cpp"),
    ("stream", "test_lilac_asm_pcm_stream.cpp"),
    ("afe", "test_lilac_afe_slimbus_playback.cpp"),
    ("session", "test_lilac_pcm_playback_session.cpp"),
    ("transport", "test_lilac_pcm_playback_transport.cpp"),
    ("fifo", "test_lilac_apr_smem_fifo.cpp"),
    ("smem", "test_lilac_pcm_smem_transport.cpp"),
    ("runtime", "test_lilac_pcm_runtime.cpp"),
)
SUPPORT = (
    "LilacAsmPcm.h", "LilacAsmPcmQueue.h", "LilacAsmPcmApr.h",
    "LilacAsmPcmMap.h", "LilacAsmPcmStream.h",
    "LilacAdmPlaybackDevice.h", "LilacAdmPlaybackRoute.h",
    "LilacAfeSlimbusPlayback.h", "LilacPcmPlaybackSession.h",
    "LilacPcmPlaybackTransport.h",
    "LilacAprSmemFifo.h",
    "LilacPcmPlaybackRuntime.h",
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
                     "tools/build/compile_lilac_pcm_session.cpp", "tools/build/compile_lilac_pcm_transport.cpp",
                     "tools/build/compile_lilac_pcm_smem.cpp", "tools/build/compile_lilac_pcm_runtime.cpp",
                     "tools/build/LilacPcmSmemPeerFixture.h"]
    source_files = {path: sha(ROOT / path) for path in source_paths}
    output.mkdir(parents=True)
    headers = materialize_headers(output, FIFO_HEADERS)

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
        if name == "transport" and "Lilac PCM transport PASS:" not in text:
            raise ValueError("Actual GLINK PCM transport test did not complete")
        if name == "fifo" and "Lilac APR SMEM FIFO PASS:" not in text:
            raise ValueError("Actual callback FIFO test did not complete")
        if name == "smem" and "Lilac PCM SMEM chain PASS:" not in text:
            raise ValueError("Actual callback FIFO/GLINK PCM chain test did not complete")
        if name == "runtime" and "Lilac PCM runtime PASS:" not in text:
            raise ValueError("Actual retained PCM runtime test did not complete")
        commands.extend([command, execution])
        outputs[name] = {
            "binary_sha256": sha(binary),
            "test_log_sha256": sha(output / (name + "-test.log")),
            "ASan_UBSan_passed": True,
        }
    target_objects = {}
    if arm64_object:
        sdk_command = ["xcrun", "--sdk", "iphoneos", "--show-sdk-path"]
        sdk = run(output, sdk_command, "arm64-sdk").strip()
        if not Path(sdk).is_dir():
            raise ValueError("iOS SDK directory not found")
        commands.append(sdk_command)
        for name in ("session", "transport", "smem", "runtime"):
            binary = output / ("pcm-" + name + ".arm64.o")
            command = ["xcrun", "--sdk", "iphoneos", "clang++", "-target", "arm64-apple-ios16.7",
                       "-isysroot", sdk, "-std=c++17", "-O1", "-Wall", "-Wextra", "-Werror",
                       "-ffreestanding", "-fno-exceptions", "-fno-rtti", "-I", str(output), "-c",
                       str(ROOT / ("tools/build/compile_lilac_pcm_" + name + ".cpp")), "-o", str(binary)]
            run(output, command, "arm64-" + name + "-compile")
            file_command, symbols_command = ["file", str(binary)], ["nm", "-u", str(binary)]
            description = run(output, file_command, "arm64-" + name + "-file")
            undefined = run(output, symbols_command, "arm64-" + name + "-undefined")
            if "Mach-O 64-bit object arm64" not in description:
                raise ValueError("Expected an ARM64 Mach-O object")
            commands.extend([command, file_command, symbols_command])
            target_objects[name] = dict(sha256=sha(binary), description=description.strip(),
                                       undefined_symbols=undefined.splitlines(), linked_into_kernel=False)
    if source_files != {path: sha(ROOT / path) for path in source_paths}:
        raise ValueError("Playback sources changed during verification")
    report = dict(
        revision=4, source_files={**headers, **source_files}, commands=commands, tests=outputs,
        arm64_objects=target_objects,
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
        actual_locked_GLINK_transport_used_with_authored_FIFO=True,
        immutable_APR_busy_retry_without_new_token=True,
        incomplete_APR_transaction_consumed_without_link_fault=True,
        direct_session_sink_Map_basic_ACK_fault_reproduced=True,
        command_and_write_completions_require_prior_publication=True,
        fragmented_GLINK_response_waits_for_complete_APR_packet=True,
        ambiguous_FIFO_publication_not_retried_or_released=True,
        clock_regression_epoch_change_and_readiness_loss_stop_progress=True,
        original_publish_reply_write_and_close_deadlines_not_extended=True,
        GLINK_close_ack_and_remote_close_required_before_region_release=True,
        concrete_SMEM_callback_FIFO_used_with_host_memory=True,
        all_2048_TX_and_RX_ring_positions_verified=True,
        AP_only_cursors_and_exact_16_byte_TX_reserve_verified=True,
        staged_RX_and_FIFO_barrier_publication_notification_order_verified=True,
        complete_wrapped_PCM_GLINK_FIFO_chain_passed=True,
        retained_runtime_owns_link_readiness_and_four_PCM_spans=True,
        PCM_bytes_and_cache_release_precede_APR_publication=True,
        matched_completion_and_cache_acquire_precede_CPU_reuse=True,
        overlapping_PCM_copy_and_foreign_completion_ownership_checked=True,
        real_clock_rechecked_during_receive_poll_send_and_cache_callbacks=True,
        late_reply_late_close_and_blocked_cache_callback_deadlines_refused=True,
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
