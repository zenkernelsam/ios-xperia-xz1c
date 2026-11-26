"""Build the offline AFE protocol test from the formal patch and source lock."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HEADERS = ("LilacAPRProtocol.h", "LilacAPRBasebandProtocol.h")
LINK_HEADERS = HEADERS + ("LilacAPRBasebandGLINK.h", "LilacAPRLink.h")
FIFO_HEADERS = LINK_HEADERS + ("ADSPAPRProbe.h",)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def materialize_headers(output, headers=HEADERS):
    if not headers or len(set(headers)) != len(headers) or any(name not in FIFO_HEADERS for name in headers):
        raise ValueError("Use unique locked APR/GLINK header names")
    lock = json.loads((ROOT / "reproducibility/source-lock.json").read_bytes())
    patch = (ROOT / "xnu/lilac-current.patch").read_bytes()
    if digest(patch) != lock["xnu"]["patch_sha256"]:
        raise ValueError("Use the locked formal XNU patch")
    sections = patch.split(b"diff --git ")
    sources = {}
    for name in headers:
        path = "iokit/Kernel/arm/" + name
        prefix = ("a/" + path + " b/" + path + "\n").encode()
        matches = [s for s in sections if s.startswith(prefix)]
        if len(matches) != 1 or b"--- /dev/null\n" not in matches[0]:
            raise ValueError("Expected one original APR added file: " + name)
        lines = []
        for line in matches[0].splitlines(keepends=True):
            if line.startswith(b"+") and not line.startswith(b"+++"):
                lines.append(line[1:])
            elif line.startswith((b"-", b" ")) and not line.startswith((b"---", b"index ")):
                raise ValueError("Unexpected original APR patch context")
        data = b"".join(lines)
        if digest(data) != lock["audited_xnu_files"][path]:
            endings = lock["xnu"]["baseline_source_newlines"]["sources"].get(path)
            if endings and digest(data) == endings["before_sha256"]:
                native_lines = data.splitlines()
                if b"\r" in data or len(native_lines) != endings["line_count"]:
                    raise ValueError("Original APR source-line layout differs: " + name)
                lf_only = set(endings["lf_only_indices"])
                data = b"".join(line + (b"\n" if index in lf_only else b"\r\n")
                                for index, line in enumerate(native_lines))
                if digest(data) != endings["after_sha256"] or digest(data) != lock["audited_xnu_files"][path]:
                    raise ValueError("Original APR native newline reconstruction differs: " + name)
            else:
                crlf = data.replace(b"\n", b"\r\n")
                if b"\r" in data or digest(crlf) != lock["audited_xnu_files"][path]:
                    raise ValueError("Original APR header digest differs: " + name)
                # Restore line endings only when the exact form is locked.
                data = crlf
        with (output / name).open("xb") as stream:
            stream.write(data)
        sources[path] = digest(data)
    return sources


def run(output, command, name):
    result = subprocess.run(command, capture_output=True, timeout=60)
    with (output / (name + ".log")).open("xb") as stream:
        stream.write(result.stdout + result.stderr)
    if result.returncode:
        raise ValueError(name + " failed: " + result.stderr.decode("utf-8", "replace")[-2000:])
    return result.stdout.decode("utf-8", "replace")


def check(output):
    output = Path(output).resolve()
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output under formal build/")
    output.mkdir(parents=True)
    sources = materialize_headers(output)
    source = ROOT / "tools/build/test_lilac_afe_slimbus_playback.cpp"
    header = ROOT / "xnu/support/LilacAfeSlimbusPlayback.h"
    if os.name == "nt":
        def linux_path(path):
            path = Path(path).resolve()
            return "/mnt/" + path.drive[0].lower() + "/" + path.as_posix()[3:]
        command = ["wsl.exe", "-d", "Ubuntu-22.04", "--", "clang++", "-std=c++17", "-O1", "-g",
                   "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                   "-fno-sanitize-recover=all", "-no-pie", "-I", linux_path(output),
                   linux_path(source), "-o", linux_path(output / "test_afe")]
        execution = ["wsl.exe", "-d", "Ubuntu-22.04", "--", linux_path(output / "test_afe")]
    else:
        command = ["clang++", "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                   "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                   "-I", str(output), str(source), "-o", str(output / "test_afe")]
        execution = [str(output / "test_afe")]
    run(output, command, "compile")
    text = run(output, execution, "test")
    if "Lilac AFE SLIMBUS PASS:" not in text:
        raise ValueError("Actual AFE test completion missing")
    report = dict(source_files={**sources, source.relative_to(ROOT).as_posix(): digest(source.read_bytes()),
                                header.relative_to(ROOT).as_posix(): digest(header.read_bytes()),
                                "tools/build/check_lilac_afe_slimbus.py": digest(Path(__file__).read_bytes())},
                  commands=[command, execution], host_ASan_UBSan_passed=True,
                  config_bytes=36, SLIMBUS_member_bytes=24, config_packets_V2_V3=[84, 92],
                  APR_source_and_destination_ports=0, start_and_stop_packet_bytes=24,
                  codec_channels_and_parameter_schema_require_separate_validation=True,
                  transport_connected=False, linked_into_device_kernel=False,
                  DSP_or_codec_or_physical_audio_verified=False, device_operations=0)
    with (output / "manifest.json").open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    print(json.dumps(check(args.output)))
