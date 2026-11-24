"""Replay the pinned Mac v730 build into a new directory, replacing one object."""

import argparse
import hashlib
import json
import shlex
import struct
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import audit


KERNEL_SHA = "f258dee9c6d142b90a573f796eeec5e92b39dd65feed6a21b2542d90fb66a88d"
LOG_SHA = "4e6aea72ef66a6dbe049db1a8976fa770d46e3ec2c0b39bed85e4b39b57f7c64"
LIST_SHA = "5650f24cd97020e811aa6f9fb67308ecb40f077bb941844d8f2831043e0dc11a"
OVERLAY_SHA = "ac01002ebf980bad04b9367ffe93198359b1325efe327d372cd5447845b56d6e"
OBJECT_SHA = "d59b8ad6402962afbdea286c1cc6cba31be89f1b1b5c72ab9b0708249b196822"
CLANG = "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang++"
ARM = Path("iokit/Kernel/arm")
FRAMEBUFFER = "SweetFramebuffer.cpp"
OBJECT = "SweetFramebuffer.cpo"
DEPS = "SweetFramebuffer.cpd"
LINK_OUTPUT = "kernel.development.sweet.unstripped"
LINK_OBJECTS = ("version.o", "lastkerneldataconst.o", "lastkernelconstructor.o", "nonlto.o")


def sha(path):
    state = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            state.update(block)
    return state.hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def command(log, predicate):
    lines = [line for line in log.splitlines() if line.startswith(CLANG + " ") and predicate(line)]
    require(len(lines) == 1, "Expected one exact v730 build recipe")
    return shlex.split(lines[0])


def replace_arg(argv, flag, old, new):
    require(argv.count(flag) == 1, f"Unexpected number of {flag} flags")
    index = argv.index(flag) + 1
    require(argv[index] == str(old), f"Unexpected {flag} value")
    argv[index] = str(new)


def uuid_bounds(data):
    require(len(data) >= 32 and data[:4] == b"\xcf\xfa\xed\xfe", "Expected 64-bit little-endian Mach-O")
    count, commands_bytes = struct.unpack_from("<II", data, 16)
    cursor, end = 32, 32 + commands_bytes
    require(end <= len(data) and count <= commands_bytes // 8, "Invalid Mach-O command table")
    matches = []
    for _ in range(count):
        require(cursor + 8 <= end, "Truncated Mach-O command")
        kind, size = struct.unpack_from("<II", data, cursor)
        require(size >= 8 and cursor + size <= end, "Invalid Mach-O command extent")
        if kind == 0x1b:
            require(size == 24, "Invalid LC_UUID size")
            matches.append((cursor + 8, cursor + 24))
        cursor += size
    require(cursor == end and len(matches) == 1, "Expected exactly one Mach-O UUID")
    return matches[0]


def build(root, overlay, output):
    require(sys.platform == "darwin", "Use the Mac v730 build host")
    require((Path.home() / "xnu-autobuild.pause").is_file(), "Keep the autobuilder paused")
    require(output.is_relative_to(root / "build") and not output.exists(), "Use a new build destination")
    object_root = root / "build/dxb/build/xnu.obj/DEVELOPMENT_ARM64_SWEET"
    compile_root = object_root / "iokit/DEVELOPMENT"
    original_object = compile_root / OBJECT
    original_source = root / "build/dxb/xnu" / ARM / FRAMEBUFFER
    build_log = root / "build/surface-diag-v730/build.log"
    filelist = object_root / "link.filelist"
    overlay_manifest = overlay / "manifest.json"
    for path, expected in ((build_log, LOG_SHA), (filelist, LIST_SHA), (original_object, OBJECT_SHA),
                           (object_root / LINK_OUTPUT, KERNEL_SHA), (overlay_manifest, OVERLAY_SHA)):
        require(sha(path) == expected, f"Input changed: {path}")
    source_proof = audit(730, KERNEL_SHA)
    overlay_files = json.loads(overlay_manifest.read_bytes())["overlay_sources"]
    require(set(overlay_files) == {str(ARM / name) for name in (
        "SweetFramebufferScreenRuntime.h", "LilacWledSnapshot.h", "LilacWledScreenProbe.h")},
        "Unexpected WLED overlay inputs")
    contents = {name: (overlay / "overlay" / name).read_bytes() for name in overlay_files}
    require(all(hashlib.sha256(data).hexdigest() == overlay_files[name] for name, data in contents.items()),
            "WLED overlay source bytes changed")
    log = build_log.read_text(encoding="utf-8", errors="replace")
    compile_args = command(log, lambda line: f" -o {OBJECT} -c " in line and line.endswith(str(original_source)))
    link_args = command(log, lambda line: " -filelist link.filelist " in line and f"/{LINK_OUTPUT} " in line)
    inputs = filelist.read_text().splitlines()
    require(inputs.count(str(original_object)) == 1 and len(inputs) == len(set(inputs)),
            "Link inputs do not identify exactly one framebuffer object")
    require(all(Path(name).is_absolute() and Path(name).resolve().is_relative_to(object_root) for name in inputs),
            "Unexpected link filelist path")
    link_paths = [Path(name) for name in inputs] + [object_root / name for name in LINK_OBJECTS]
    original_link_hashes = {str(path): sha(path) for path in link_paths}

    output.mkdir()
    source = output / FRAMEBUFFER
    source.write_bytes(original_source.read_bytes())
    for name, data in contents.items():
        (output / Path(name).name).write_bytes(data)

    def run(argv, cwd, stage):
        (output / f"{stage}.argv.json").write_text(json.dumps(argv, indent=2) + "\n", encoding="utf-8")
        with (output / f"{stage}.log").open("xb") as stream:
            subprocess.run(argv, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT, timeout=300, check=True)

    baseline = output / "baseline.relinked.macho"
    link_before = link_args.copy()
    original_link_target = link_args[link_args.index("-o") + 1]
    require(Path(original_link_target).resolve() == object_root / LINK_OUTPUT, "Unexpected original link destination")
    replace_arg(link_before, "-o", original_link_target, baseline)
    run(link_before, object_root, "baseline-link")
    preserved = (object_root / LINK_OUTPUT).read_bytes()
    replay = baseline.read_bytes()
    start, end = uuid_bounds(preserved)
    require(uuid_bounds(replay) == (start, end) and preserved[:start] == replay[:start]
            and preserved[end:] == replay[end:], "Original objects differ outside the linker-generated LC_UUID")
    print("Baseline v730 relink matches every byte outside LC_UUID", flush=True)

    compile_args[-1:] = ["-iquote", str(original_source.parent), str(source)]
    replace_arg(compile_args, "-o", OBJECT, output / OBJECT)
    replace_arg(compile_args, "-MF", DEPS, output / DEPS)
    run(compile_args, compile_root, "wled-compile")
    dep_text = (output / DEPS).read_text()
    require(all(str(output / Path(name).name) in dep_text for name in contents),
            "Compiler did not consume the isolated WLED headers")
    new_object = output / OBJECT
    new_list = output / "wled.link.filelist"
    new_list.write_text("\n".join(str(new_object) if name == str(original_object) else name for name in inputs) + "\n")
    candidate = output / "kernel.wled-observer.macho"
    link_after = link_args.copy()
    replace_arg(link_after, "-filelist", "link.filelist", new_list)
    replace_arg(link_after, "-o", original_link_target, candidate)
    run(link_after, object_root, "wled-link")
    marker = b"SWEET WLED SAMPLE token="
    require(marker in new_object.read_bytes() and marker in candidate.read_bytes() and marker not in baseline.read_bytes(),
            "Compiled WLED marker is not uniquely linked into the candidate")
    require(all(sha(Path(name)) == digest for name, digest in original_link_hashes.items()),
            "An original link input changed during the build")
    require(audit(730, KERNEL_SHA) == source_proof, "The v730 source or kernel changed")
    report = {
        "base_kernel_sha256": KERNEL_SHA,
        "baseline_relink_sha256": sha(baseline),
        "baseline_uuid_range": [start, end],
        "baseline_original_uuid": preserved[start:end].hex(),
        "baseline_relinked_uuid": replay[start:end].hex(),
        "baseline_all_other_bytes_exact": True,
        "base_source_audit": source_proof,
        "original_build_log_sha256": LOG_SHA,
        "original_filelist_sha256": LIST_SHA,
        "original_link_inputs_sha256": original_link_hashes,
        "overlay_manifest_sha256": OVERLAY_SHA,
        "overlay_sources_sha256": overlay_files,
        "original_framebuffer_object_sha256": OBJECT_SHA,
        "candidate_framebuffer_object_sha256": sha(new_object),
        "candidate_kernel_sha256": sha(candidate),
        "candidate_kernel_bytes": candidate.stat().st_size,
        "only_link_object_replaced": str(original_object.relative_to(root)),
        "host_link_verified": True,
        "live_admission": False,
        "device_operations": 0,
    }
    (output / "link-proof.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"WLED kernel linked: {report['candidate_kernel_sha256']}", flush=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, required=True)
    parser.add_argument("--overlay", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build(args.repository.resolve(), args.overlay.resolve(), args.output.resolve())
