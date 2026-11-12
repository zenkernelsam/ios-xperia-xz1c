"""Transport fault replay: no USB device, no serial and no recovery command."""
import hashlib
import json
import struct
import tempfile
import contextlib
import unittest
from pathlib import Path
from boot_once import BootOnce, CHUNK, load_image

IMAGE = b"x" * 4096
DATA = b"DATA00001000"

class Wire:
    def __init__(self, statuses, fail_kind=None, short_payload=False, hook=None):
        self.statuses = list(statuses)
        self.calls = []
        self.fail_kind, self.short_payload, self.hook = fail_kind, short_payload, hook
    def write(self, data, timeout):
        self.calls.append(("write", bytes(data)))
        kind = "boot" if data == b"boot" else "download" if data.startswith(b"download:") else "payload"
        if kind == self.fail_kind:
            raise OSError("Pipe error ep=0x01")
        return 13 if kind == "payload" and self.short_payload else len(data)
    def read(self, size, timeout):
        self.calls.append(("read", size))
        response = self.statuses.pop(0)
        if self.hook: self.hook(response)
        if isinstance(response, Exception): raise response
        return response
    def writes(self, value):
        return sum(op == "write" and data == value for op, data in self.calls)

class Tests(unittest.TestCase):
    def run_wire(self, wire, image=IMAGE, **kw):
        runner = BootOnce(wire, **kw)
        return runner, runner.run(image)
    def test_ack_is_not_kernel_live(self):
        wire = Wire([DATA, b"OKAY", b"OKAY"])
        _, row = self.run_wire(wire)
        self.assertEqual(row["result"], "BOOT_ACKNOWLEDGED")
        self.assertTrue(row["payload_acknowledged"])
        self.assertFalse(row["kernel_live_verified"])
        self.assertEqual(wire.writes(b"boot"), 1)
    def test_actual_timeout_never_reissues_boot(self):
        wire = Wire([DATA, b"OKAY", OSError("Operation timed out ep=0x81 len=0x0040")])
        _, row = self.run_wire(wire)
        self.assertEqual(row["result"], "BOOT_UNCERTAIN")
        self.assertEqual(row["boot_command_attempts"], 1)
        self.assertTrue(row["boot_write_completed"])
        self.assertEqual(wire.writes(b"boot"), 1)
        self.assertEqual(wire.calls[-1][0], "read")
        self.assertEqual(row["retries"], 0)
    def test_boot_write_error_has_one_attempt(self):
        wire = Wire([DATA, b"OKAY"], fail_kind="boot")
        _, row = self.run_wire(wire)
        self.assertEqual(row["boot_command_attempts"], 1)
        self.assertFalse(row["boot_write_completed"])
        self.assertEqual(wire.writes(b"boot"), 1)
    def test_download_timeout_does_not_repeat_or_boot(self):
        wire = Wire([OSError("timeout")])
        _, row = self.run_wire(wire)
        self.assertEqual(row["download_command_attempts"], 1)
        self.assertEqual(row["boot_command_attempts"], 0)
        self.assertEqual(len([1 for op, _ in wire.calls if op == "write"]), 1)
    def test_wrong_download_length_has_no_payload_or_boot(self):
        wire = Wire([b"DATA00002000"])
        _, row = self.run_wire(wire)
        self.assertEqual(row["payload_bytes_written"], 0)
        self.assertEqual(row["boot_command_attempts"], 0)
    def test_rejected_download(self):
        wire = Wire([b"FAILdenied"])
        _, row = self.run_wire(wire)
        self.assertEqual(row["result"], "STOP_BEFORE_BOOT")
        self.assertEqual(wire.writes(b"boot"), 0)
    def test_short_payload_no_repeat_known_bytes(self):
        wire = Wire([DATA], short_payload=True)
        _, row = self.run_wire(wire)
        self.assertEqual(row["payload_bytes_written"], 13)
        self.assertEqual(row["boot_command_attempts"], 0)
        self.assertEqual(len([1 for op, _ in wire.calls if op == "write"]), 2)
    def test_payload_error_lower_bound(self):
        wire = Wire([DATA], fail_kind="payload")
        _, row = self.run_wire(wire)
        self.assertTrue(row["payload_bytes_are_lower_bound"])
        self.assertEqual(row["boot_command_attempts"], 0)
    def test_payload_ack_timeout_no_boot(self):
        wire = Wire([DATA, OSError("ack timeout")])
        _, row = self.run_wire(wire)
        self.assertEqual(row["payload_bytes_written"], 4096)
        self.assertFalse(row["payload_acknowledged"])
        self.assertEqual(wire.writes(b"boot"), 0)
    def test_info_is_receive_only(self):
        wire = Wire([b"INFOone", DATA, b"INFOtwo", b"OKAY", b"INFOthree", b"OKAY"])
        _, row = self.run_wire(wire)
        self.assertEqual(row["result"], "BOOT_ACKNOWLEDGED")
        self.assertEqual(row["download_command_attempts"], 1)
        self.assertEqual(wire.writes(b"boot"), 1)
    def test_too_many_info_no_boot_resend(self):
        wire = Wire([DATA, b"OKAY"] + [b"INFOwait"] * 128)
        _, row = self.run_wire(wire)
        self.assertEqual(row["result"], "BOOT_UNCERTAIN")
        self.assertEqual(wire.writes(b"boot"), 1)
    def test_malformed_status_no_boot(self):
        for status in (b"", b"OK", b"WHAT", b"DATA" + b"0" * 65):
            wire = Wire([status])
            _, row = self.run_wire(wire)
            self.assertEqual(row["boot_command_attempts"], 0)
    def test_deadline_before_boot_no_command_attempt(self):
        clock = [0.0]
        def hook(response):
            if response == b"OKAY": clock[0] = 46
        wire = Wire([DATA, b"OKAY"], hook=hook)
        _, row = self.run_wire(wire, clock=lambda: clock[0])
        self.assertEqual(row["boot_command_attempts"], 0)
        self.assertEqual(wire.writes(b"boot"), 0)
    def test_large_chunks_exact_no_duplicate(self):
        image = b"x" * (CHUNK + 4096)
        wire = Wire([("DATA%08x" % len(image)).encode(), b"OKAY", b"OKAY"])
        _, row = self.run_wire(wire, image)
        sizes = [len(data) for op, data in wire.calls if op == "write" and len(data) > 64]
        self.assertEqual(sizes, [CHUNK, 4096])
        self.assertEqual(row["payload_bytes_written"], len(image))
    def test_second_run_cannot_send_again(self):
        wire = Wire([DATA, b"OKAY", b"OKAY"])
        runner, _ = self.run_wire(wire)
        before = list(wire.calls)
        with self.assertRaises(AssertionError): runner.run(IMAGE)
        self.assertEqual(wire.calls, before)
    def test_trace_error_after_boot_keeps_actual_count(self):
        def trace(**event):
            if event["event"] == "write_return" and event.get("kind") == "boot":
                raise OSError("trace disk error")
        wire = Wire([DATA, b"OKAY", b"OKAY"])
        _, row = self.run_wire(wire, trace=trace)
        self.assertEqual(row["result"], "BOOT_UNCERTAIN")
        self.assertEqual(row["boot_command_attempts"], 1)
        self.assertTrue(row["boot_write_completed"])
        self.assertEqual(wire.writes(b"boot"), 1)
    def test_image_validation_before_any_transport(self):
        data = bytearray(12288)
        data[:8] = b"ANDROID!"
        struct.pack_into("<I", data, 8, 4096)
        struct.pack_into("<I", data, 16, 4096)
        struct.pack_into("<I", data, 36, 4096)
        data[8192:8200] = b"SWEETXNU"
        # Preserve temporary fixtures for the native recycle workflow.
        with contextlib.nullcontext(tempfile.mkdtemp(prefix="lilac-boot-input-")) as folder:
            path = Path(folder) / "input.img"
            path.write_bytes(data)
            digest = hashlib.sha256(data).hexdigest()
            self.assertEqual(load_image(path, digest), data)
            with self.assertRaises(ValueError): load_image(path, "0" * 64)

if __name__ == "__main__":
    unittest.main(verbosity=2)
