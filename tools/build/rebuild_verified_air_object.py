"""Rebuild a byte-identical AIR object from its retained original import audit."""

import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

from materialize_air_sources import materialize, root

REFERENCE_MANIFEST_SHA = "29e024138f44e84ef32e89b1d872521f88130e8cab413843975865f56b6790e0"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_reference(manifest, repository=root):
    required = (
        "actual_AIR_stage_translation_verified",
        "forced_legacy_interface_host_test_passed",
        "host_asan_ubsan_fixture_passed",
        "ios_arm64_compilation_passed",
    )
    if not all(manifest.get(name) is True for name in required):
        raise ValueError("Original AIR compilation and translation proof is incomplete")
    audit = manifest["llvm_import_audit"]
    expected = manifest["compiled_AIR_object_sha256"]
    if not re.fullmatch(r"[0-9a-f]{64}", expected) or audit["object_sha256"] != expected:
        raise ValueError("Original object and import audit differ")
    if audit.get("exact_compiled_imports_checked") is not True or audit["missing_target_imports"]:
        raise ValueError("Original target import audit is incomplete")
    if not re.fullmatch(r"[0-9a-f]{64}", audit["target_symbols_sha256"]):
        raise ValueError("Original target exports identity is missing")
    if audit.get("target") != "iOS16.7.16/20H392 libLLVM.dylib":
        raise ValueError("Only the retained original iOS target audit is admitted")
    if not audit["imports"] or audit["imports"] != sorted(set(audit["imports"])):
        raise ValueError("Original LLVM import list is incomplete")
    if not manifest["source_files"]:
        raise ValueError("Original canonical AIR source map is empty")
    for name, digest in manifest["source_files"].items():
        source = (repository / name).resolve()
        if not source.is_relative_to(repository.resolve()) or sha(source) != digest:
            raise ValueError("Canonical AIR source differs: " + name)


def verify_object(path, manifest, undefined_symbols):
    if sha(path) != manifest["compiled_AIR_object_sha256"]:
        raise ValueError("Rebuilt AIR object differs; a new target import audit is required")
    imports = sorted(set(re.findall(r"\b_(LLVM[A-Za-z0-9_]+)\b", undefined_symbols)))
    if imports != manifest["llvm_import_audit"]["imports"]:
        raise ValueError("Rebuilt AIR LLVM imports differ from the original target audit")
    return imports


def rebuild(reference, headers, output):
    reference = Path(reference).resolve()
    headers = Path(headers).resolve()
    output = Path(output).resolve()
    if not reference.is_relative_to(root) or not headers.is_relative_to(root / "deps"):
        raise ValueError("Use formal repository inputs and fixed dependency headers")
    if not (headers / "llvm-c/Core.h").is_file():
        raise ValueError("LLVM C headers are missing")
    if not output.is_relative_to(root / "build") or output.exists():
        raise ValueError("Use a new output directory under build/")
    original = reference.read_bytes()
    if hashlib.sha256(original).hexdigest() != REFERENCE_MANIFEST_SHA:
        raise ValueError("Original reviewed AIR reference manifest differs")
    manifest = json.loads(original)
    verify_reference(manifest)
    output.mkdir(parents=True)
    sources = materialize(output / "sources")
    obj = output / "AIRFrontend.ios.o"
    command = [
        "xcrun", "--sdk", "iphoneos", "clang++", "-arch", "arm64",
        "-miphoneos-version-min=16.0", "-std=c++17", "-DSWEET_LLVM_PRE23=1",
        "-I", str(output / "sources"), "-I", str(headers), "-O2", "-c",
        str(output / "sources/Frontend.cpp"), "-o", str(obj),
    ]
    compiled = subprocess.run(command, cwd=root, capture_output=True, text=True)
    (output / "ios-compile.log").write_text(compiled.stdout + compiled.stderr)
    if compiled.returncode:
        raise RuntimeError(compiled.stdout + compiled.stderr)
    symbols = subprocess.run(["nm", "-u", str(obj)], capture_output=True, text=True, check=True).stdout
    (output / "undefined-symbols.txt").write_text(symbols)
    imports = verify_object(obj, manifest, symbols)
    verify_reference(manifest)
    if reference.read_bytes() != original:
        raise ValueError("Original reference changed during compilation")
    # This preserves the original audit only for the exact same object bytes.
    (output / "manifest.json").write_bytes(original)
    report = dict(
        reference_manifest=reference.relative_to(root).as_posix(),
        reference_manifest_sha256=hashlib.sha256(original).hexdigest(),
        builder_sha256=sha(Path(__file__).resolve()),
        canonical_source_files_verified=len(manifest["source_files"]),
        materialized_source_binding=sources,
        compile_command=command,
        object_sha256=sha(obj),
        original_object_bytes_identical=True,
        original_audited_LLVM_imports=len(imports),
        original_target_exports_sha256=manifest["llvm_import_audit"]["target_symbols_sha256"],
        original_target_audit_preserved_without_rereading_exports=True,
        different_object_or_new_target_not_admitted=True,
        device_operations=0,
    )
    (output / "reconstruction.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-manifest", required=True)
    parser.add_argument("--llvm-headers", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    result = rebuild(args.reference_manifest, args.llvm_headers, args.output)
    print(json.dumps({name: result[name] for name in (
        "object_sha256", "original_object_bytes_identical", "original_audited_LLVM_imports", "device_operations"
    )}))
