"""Tests for scanout-bound, same-LIVE WLED evidence parsing."""
import binascii
import hashlib
import json
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))
import lilac_wled_evidence as evidence
import trial_evidence


TOKEN = "0123456789abcdef"
IMAGE = "a" * 64
LIVE = f"SWEET LOOP LIVE proto=1 token={TOKEN} mode=graceful pon=0x06"
ACK = f"SWEET LOOP ACK FASTBOOT token={TOKEN} mode=graceful pon=0x06"
BEGIN = f"SWEET SCREEN BEGIN token={TOKEN} id=1 "
END = f"SWEET SCREEN END token={TOKEN} id=1 "
SAMPLE = f"SWEET WLED SAMPLE token={TOKEN} id=1 state=6 fault=0 enabled=1 sinks=3 modulators=3 duty0=564 duty1=564 read_only=1"
SAMPLE_RECORD = SAMPLE + f" wire_crc={binascii.crc32(SAMPLE.encode()):08x}"
RAW = (LIVE + "\n" + BEGIN + "\n" + END + "\n" + SAMPLE_RECORD + "\n" + ACK + "\n").encode()


class LilacWledEvidence(unittest.TestCase):
    def run_with(self, raw, mode="private", version=evidence.WLED_VERSION):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "cycle-01").mkdir()
            (root / "cycle-01/serial.raw.log").write_bytes(raw)
            (root / "results.json").write_text(json.dumps([{
                "result": "PASS" if mode == "private" else "REVISE",
                "artifact": {"version": version, "kernel_sha256": evidence.WLED_KERNEL_SHA,
                             "sha256": IMAGE, "trial_mode": mode},
            }]), encoding="utf-8")
            scan = {
                    "token": TOKEN, "image_sha256": IMAGE, "raw_sha256": hashlib.sha256(raw).hexdigest(),
                    "screen": {"frames": [{"id": 1}]}, "original_result": "REVISE"}
            with (patch.object(evidence, "verify_scanout", return_value=scan),
                  patch.object(evidence, "verify_trial", return_value=dict(scan, log_sha256=scan["raw_sha256"])),
                  patch.object(trial_evidence, "verify", return_value=dict(scan, log_sha256=scan["raw_sha256"]))):
                return evidence.inspect(root, IMAGE)

    def test_current_frame_sample_kept_distinct_from_optical_result(self):
        report = self.run_with(RAW)
        self.assertEqual(report["sample"]["duty"], [564, 564])
        self.assertTrue(report["same_live_as_complete_scanout"])
        self.assertEqual(report["original_trial_result"], "PASS")
        self.assertFalse(report["pwm_current_luminosity_verified"])
        self.assertFalse(report["optical_panel_verified"])

    def test_complete_scanout_without_wled_record_stays_unavailable(self):
        raw = RAW.replace((SAMPLE_RECORD + "\n").encode(), b"")
        report = self.run_with(raw)
        self.assertEqual(report["original_trial_result"], "PASS")
        self.assertEqual(report["sample"], {"status": "unavailable", "reason": "no_bounded_wled_record"})

    def test_original_gpu_revise_kept_and_v730_rejected(self):
        self.assertEqual(self.run_with(RAW, mode="gpu")["original_trial_result"], "REVISE")
        with self.assertRaisesRegex(ValueError, "exact observation artifact"):
            self.run_with(RAW, version=730)

    def test_wait_for_split_sample_after_frame_without_extending_deadline(self):
        def run(chunks, alive=True):
            chunks = iter(chunks)
            elapsed = [0.0]
            def sleep(seconds):
                elapsed[0] += seconds
            result = evidence.wait_for_sample(lambda: next(chunks, b""), lambda: alive,
                                              lambda: None, TOKEN, 0.2, lambda: elapsed[0], sleep)
            return result, elapsed[0]

        end = (END + "\n").encode()
        wire = (SAMPLE_RECORD + "\n").encode()
        self.assertEqual(run([end[:10], end[10:] + wire[:25], wire[25:]])[0], "received")
        self.assertEqual(run([end + wire.replace(b"duty0=564", b"duty0=565")])[0], "invalid")
        self.assertEqual(run([wire + end]), ("unavailable", 0.2))
        self.assertEqual(run([end], alive=False), ("unavailable", 0.0))

    def test_refuses_missing_duplicate_or_outside_frame_sample(self):
        missing = RAW.replace((SAMPLE_RECORD + "\n").encode(), b"")
        report = self.run_with(missing)
        self.assertEqual(report["sample"]["status"], "unavailable")
        duplicate = RAW.replace((SAMPLE_RECORD + "\n").encode(),
                                (SAMPLE_RECORD + "\n" + SAMPLE_RECORD + "\n").encode())
        outside = RAW.replace((SAMPLE_RECORD + "\n").encode(), b"").replace(
            BEGIN.encode(), (SAMPLE_RECORD + "\n" + BEGIN).encode())
        for raw in (duplicate, outside):
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                self.run_with(raw)

    def test_refuses_foreign_or_out_of_range_register_values(self):
        damaged = [SAMPLE.replace(old, new) for old, new in (
            (TOKEN, "f" * 16), ("duty0=564", "duty0=4096"), ("sinks=3", "sinks=16"),
            ("read_only=1", "read_only=0"), ("state=6", "state=3"))]
        cases = [RAW.replace(SAMPLE_RECORD.encode(), f"{sample} wire_crc={binascii.crc32(sample.encode()):08x}".encode())
                 for sample in damaged]
        cases.append(RAW.replace(b"duty0=564", b"duty0=565"))
        for raw in cases:
            with self.subTest(raw=raw), self.assertRaises(ValueError):
                self.run_with(raw)


if __name__ == "__main__":
    unittest.main()
