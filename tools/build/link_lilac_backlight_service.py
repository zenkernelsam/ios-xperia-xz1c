"""Compile and link the unregistered backlight service beside the exact v731 inputs."""

import argparse
import json
import subprocess
import sys
from pathlib import Path

from link_lilac_wled_candidate import ARM, LINK_OBJECTS, require, replace_arg, sha, uuid_bounds
from diagnostic_candidate import ROOT, WLED_BUILD, WLED_KERNEL_SHA, WLED_LINK_PROOF_SHA, audit


RECIPES = {
    "wled-compile.argv.json": "588f3866e133bccfb849337f6e5676e965c5765316e5ae37b835f589cd80e60a",
    "wled-link.argv.json": "9e0975b9ea4c9cfb8b4778f69e967e659b92a32af624809b607cc80162ba86fd",
    "wled.link.filelist": "a861ce2a6506fce38d23f90bd3b4fd77523a5a72220930fae7bc7ef6784c30b5",
}
SOURCES = ("AppleARMBacklight.cpp", "LilacBacklightService.h", "LilacBacklightProperties.h")


def build(source_directory, output):
    require(sys.platform == "darwin" and (Path.home() / "xnu-autobuild.pause").is_file(),
            "Use the Mac build host with the autobuilder paused")
    require(output.is_relative_to(ROOT / "build") and not output.exists(), "Use a new build destination")
    base = ROOT / WLED_BUILD
    original_kernel = base / "kernel.wled-observer.macho"
    object_root = ROOT / "build/dxb/build/xnu.obj/DEVELOPMENT_ARM64_SWEET"
    compile_root = object_root / "iokit/DEVELOPMENT"
    audit(731, WLED_KERNEL_SHA)
    require(all(sha(base / name) == expected for name, expected in RECIPES.items()), "v731 build recipe changed")
    receipt = base / "link-proof.json"
    require(sha(receipt) == WLED_LINK_PROOF_SHA, "v731 link receipt changed")
    proof = json.loads(receipt.read_bytes())
    original_object = compile_root / "SweetFramebuffer.cpo"
    base_object = base / original_object.name
    filelist = base / "wled.link.filelist"
    files = filelist.read_text().splitlines()
    require(files.count(str(base_object)) == 1 and len(files) == len(set(files)), "Unexpected v731 filelist")
    base_hashes = dict(proof["original_link_inputs_sha256"])
    require(set(base_hashes) == ({str(original_object) if entry == str(base_object) else entry for entry in files}
                                | {str(object_root / name) for name in LINK_OBJECTS}),
            "v731 input inventory differs")
    require(all(Path(path).resolve().is_relative_to(object_root) for path in base_hashes),
            "v731 input outside preserved object root")
    base_hashes[str(base_object)] = proof["candidate_framebuffer_object_sha256"]

    def check_inputs():
        require(all(sha(Path(path)) == digest for path, digest in base_hashes.items()), "Preserved link object changed")
        require(sha(original_kernel) == WLED_KERNEL_SHA, "v731 kernel changed")

    check_inputs()
    output.mkdir()
    for name in SOURCES:
        (output / name).write_bytes((source_directory / name).read_bytes())

    def run(args, cwd, phase):
        (output / (phase + ".argv.json")).write_text(json.dumps(args, indent=2) + "\n")
        with (output / (phase + ".log")).open("xb") as log:
            subprocess.run(args, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, timeout=90, check=True)

    link = json.loads((base / "wled-link.argv.json").read_bytes())
    baseline = output / "baseline.relinked.macho"
    replace_arg(link, "-o", original_kernel, baseline)
    run(link, object_root, "baseline-link")
    original, replay = original_kernel.read_bytes(), baseline.read_bytes()
    start, end = uuid_bounds(original)
    require(uuid_bounds(replay) == (start, end) and original[:start] == replay[:start] and original[end:] == replay[end:],
            "v731 relink differs outside LC_UUID")
    args = json.loads((base / "wled-compile.argv.json").read_bytes())
    obj, deps = output / "AppleARMBacklight.cpo", output / "AppleARMBacklight.cpd"
    replace_arg(args, "-o", base_object, obj)
    replace_arg(args, "-MF", base / "SweetFramebuffer.cpd", deps)
    require(args[-3:] == ["-iquote", str(ROOT / "build/dxb/xnu" / ARM), str(base / "SweetFramebuffer.cpp")],
            "Unexpected end of original compile recipe")
    args[-3:] = [str(output / SOURCES[0])]
    run(args, compile_root, "service-compile")
    require(all(str(output / name) in deps.read_text() for name in SOURCES), "Wrong service headers used")
    imports = subprocess.check_output(["xcrun", "nm", "-u", str(obj)], text=True, timeout=20)
    require(not any(name in imports for name in ("sweet_spmi", "lilac_wled", "_ml_io_map")), "Unexpected hardware imports")

    candidate = output / "kernel.backlight-service.linkcheck.macho"
    new_list = output / "service.link.filelist"
    new_list.write_text("\n".join(files + [str(obj)]) + "\n")
    replace_arg(link, "-filelist", filelist, new_list)
    replace_arg(link, "-o", baseline, candidate)
    run(link, object_root, "service-link")
    symbols = subprocess.check_output(["xcrun", "nm", str(candidate)], text=True, timeout=20)
    require("__ZN17AppleARMBacklight13setPropertiesEP8OSObject" in symbols
            and "lilac_backlight_create_service" in symbols, "Service class or registration entry missing")
    check_inputs()
    report = {
        "base_kernel_sha256": WLED_KERNEL_SHA,
        "base_bytes_match_except_lc_uuid": True,
        "v731_link_inputs_unchanged": len(base_hashes),
        "compiled_sources_sha256": {name: sha(output / name) for name in SOURCES},
        "appended_object_sha256": sha(obj),
        "linked_kernel_sha256": sha(candidate),
        "linked_kernel_bytes": candidate.stat().st_size,
        "service_and_factory_symbols_found": True,
        "spmi_wled_or_mmio_imports": False,
        "publication_hook_present": False,
        "device_operations": 0,
    }
    (output / "service-link-proof.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sources", type=Path, default=ROOT / "xnu/support")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    build(args.sources.resolve(), args.output.resolve())
