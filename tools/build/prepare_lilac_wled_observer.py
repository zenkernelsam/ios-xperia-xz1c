"""Produce an isolated WLED diagnostic source overlay from exact v730 inputs."""

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ARM = Path("iokit/Kernel/arm")
RUNTIME = ARM / "SweetFramebufferScreenRuntime.h"
FRAMEBUFFER = ARM / "SweetFramebuffer.cpp"
SPMI = Path("bsd/dev/sweet_spmi.c")
HEADERS = ("LilacWledSnapshot.h", "LilacWledScreenProbe.h")
HOOK = b'    if (complete) sweet_screen_record(\n        "SWEET SCREEN END token=%s id=%u encoded=%lu blocks=%u raw_fnv=%016llx enc_crc=%08x complete=1",\n        fb->fScreenToken, fb->fScreenID, (unsigned long)transfer.offset, transfer.blocks, fb->fScreenHash, stats.crc);\n'
RECORD_END = b"    return 1;\n}\n\nvoid AppleCLCD::screenRequest()\n"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def prepare(tree, output):
    stage = json.loads((ROOT / "reproducibility/surface-diag-v730.json").read_bytes())
    baseline = json.loads((ROOT / "reproducibility/source-lock.json").read_bytes())
    paths = (FRAMEBUFFER, RUNTIME, SPMI)
    inputs = {path: (tree / path).read_bytes() for path in paths}
    expected = {path: (stage["overrides"] if path != SPMI else baseline["audited_xnu_files"])[
        path.as_posix()] for path in paths}
    if any(digest(inputs[path]) != expected[path] for path in paths):
        raise ValueError("WLED overlay needs the exact compiled v730 display and SPMI inputs")
    runtime = inputs[RUNTIME]
    if runtime.count(HOOK) != 1 or runtime.count(RECORD_END) != 1:
        raise ValueError("Expected exactly one v730 completed scanout record")
    runtime = runtime.replace(RECORD_END, RECORD_END.replace(
        b"\nvoid AppleCLCD", b'\n#include "LilacWledScreenProbe.h"\n\nvoid AppleCLCD'))
    completed = HOOK.replace(b"if (complete) sweet_screen_record(", b"if (complete && sweet_screen_record(")
    completed = completed.rstrip(b"\n;") + b") {\n        lilac_wled_screen_probe(fb->fScreenToken, fb->fScreenID);\n    }\n"
    runtime = runtime.replace(HOOK, completed)
    overlay = {RUNTIME: runtime}
    overlay.update({ARM / name: (ROOT / "xnu/support" / name).read_bytes() for name in HEADERS})
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output directory under build/")
    output.mkdir(parents=True)
    for path, data in overlay.items():
        target = output / "overlay" / path
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open("xb") as stream:
            stream.write(data)
    report = {
        "base_version": 730,
        "base_sources": {path.as_posix(): expected[path] for path in paths},
        "overlay_sources": {path.as_posix(): digest(data) for path, data in overlay.items()},
        "builder_sha256": digest(Path(__file__).read_bytes()),
        "activation": "first current-LIVE scanout only",
        "fixed_ppid_register_allowlist": {
            "0x3d8": [0x08, 0x46],
            "0x3d9": [0x46, 0x50, 0x60, 0x57, 0x58, 0x67, 0x68]
        },
        "general_pmic_read_allowlist_changed": False,
        "spmi_init_or_write_calls_added": 0,
        "device_operations": 0,
        "source_only_not_a_boot_candidate": True,
    }
    with (output / "manifest.json").open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--v730-tree", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(prepare(args.v730_tree.resolve(), args.output.resolve()), indent=2))
