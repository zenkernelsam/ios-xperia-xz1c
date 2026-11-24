"""Refuse mismatched retained audits, source inputs and rebuilt AIR objects."""

import copy
import hashlib
import tempfile
import unittest
from pathlib import Path

from rebuild_verified_air_object import verify_object, verify_reference


class VerifiedAirObjectTests(unittest.TestCase):
    def test_only_identical_object_and_imports_reuse_audit(self):
        directory = tempfile.mkdtemp(prefix="lilac-air-object-test-")
        if directory:
            obj = Path(directory) / "AIRFrontend.ios.o"
            obj.write_bytes(b"author fixture object")
            digest = hashlib.sha256(obj.read_bytes()).hexdigest()
            manifest = dict(compiled_AIR_object_sha256=digest,
                            llvm_import_audit=dict(imports=["LLVMContextCreate", "LLVMTypeOf"]))
            symbols = "                 U _LLVMTypeOf\n                 U _LLVMContextCreate\n"
            self.assertEqual(verify_object(obj, manifest, symbols), manifest["llvm_import_audit"]["imports"])
            with self.assertRaises(ValueError):
                verify_object(obj, manifest, symbols + "                 U _LLVMNewImport\n")
            with self.assertRaises(ValueError):
                verify_object(obj, manifest, "                 U _LLVMTypeOf\n")
            obj.write_bytes(b"changed object")
            with self.assertRaises(ValueError):
                verify_object(obj, manifest, symbols)

    def test_missing_or_mixed_original_proof_is_refused(self):
        directory = tempfile.mkdtemp(prefix="lilac-air-reference-test-")
        if directory:
            repository = Path(directory)
            source = repository / "AIR.cpp"
            source.write_bytes(b"author source fixture")
            digest = "1" * 64
            manifest = dict(
                actual_AIR_stage_translation_verified=True,
                forced_legacy_interface_host_test_passed=True,
                host_asan_ubsan_fixture_passed=True,
                ios_arm64_compilation_passed=True,
                compiled_AIR_object_sha256=digest,
                source_files={"AIR.cpp": hashlib.sha256(source.read_bytes()).hexdigest()},
                llvm_import_audit=dict(object_sha256=digest, imports=["LLVMTypeOf"],
                                      target_symbols_sha256="2" * 64, missing_target_imports=[],
                                      target="iOS16.7.16/20H392 libLLVM.dylib",
                                      exact_compiled_imports_checked=True),
            )
            verify_reference(manifest, repository)
            for key in ("actual_AIR_stage_translation_verified", "forced_legacy_interface_host_test_passed"):
                changed = copy.deepcopy(manifest)
                changed[key] = False
                with self.subTest(key=key), self.assertRaises(ValueError):
                    verify_reference(changed, repository)
            for key, value in (("object_sha256", "3" * 64), ("missing_target_imports", ["LLVMTypeOf"]),
                               ("exact_compiled_imports_checked", False), ("target", "different target")):
                changed = copy.deepcopy(manifest)
                changed["llvm_import_audit"][key] = value
                with self.subTest(key=key), self.assertRaises(ValueError):
                    verify_reference(changed, repository)
            source.write_bytes(b"changed source")
            with self.assertRaises(ValueError):
                verify_reference(manifest, repository)


if __name__ == "__main__":
    unittest.main()
