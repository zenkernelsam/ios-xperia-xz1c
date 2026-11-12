"""Sony-only RAM boot sender with one command write per phase and no retries.

The only Fastboot requests are download:<size> and boot. A boot response
timeout is terminal/uncertain and never causes another boot write.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import time
from pathlib import Path

SERIAL = "BH905ND69G"
VID, PID = 0x0FCE, 0x0DDE
MAX_IMAGE = 64 << 20
CHUNK = 16 << 20  # same chunk bound as the audited xperable fbusb sender


class BootTransportError(Exception):
    pass


def load_image(path, digest):
    path = Path(path).resolve()
    size = path.stat().st_size
    if not re.fullmatch(r"[0-9a-f]{64}", digest) or not 4096 <= size <= MAX_IMAGE or size % 4096:
        raise ValueError("Invalid expected SHA or bounded boot image size")
    data = path.read_bytes()
    if len(data) != size or hashlib.sha256(data).hexdigest() != digest:
        raise ValueError("Boot image SHA mismatch")
    if data[:8] != b"ANDROID!" or struct.unpack_from("<I", data, 36)[0] != 4096:
        raise ValueError("Expected the verified 4096-byte Android boot container")
    kernel, ramdisk = struct.unpack_from("<I", data, 8)[0], struct.unpack_from("<I", data, 16)[0]
    offset = 4096 + ((kernel + 4095) & ~4095)
    if not kernel or ramdisk < 680 or offset + ramdisk > size or data[offset:offset + 8] != b"SWEETXNU":
        raise ValueError("Expected the project SWEETXNU payload")
    return data


class BootOnce:
    def __init__(self, transport, trace=lambda **event: None, clock=time.monotonic):
        self.usb, self.trace, self.clock = transport, trace, clock
        self.limit = clock() + 45
        self.phase = "download"
        self.report = {"serial": SERIAL, "download_command_attempts": 0, "boot_command_attempts": 0,
                       "boot_write_completed": False, "payload_bytes_written": 0,
                       "payload_acknowledged": False, "payload_bytes_are_lower_bound": False,
                       "retries": 0, "kernel_live_verified": False}

    def timeout(self):
        remaining = self.limit - self.clock()
        if remaining <= 0:
            raise BootTransportError("Whole transfer deadline reached")
        return max(1, min(5000, int(remaining * 1000)))

    def write(self, data, kind):
        if kind == "boot":
            if self.report["boot_command_attempts"]:
                raise AssertionError("A second boot write is forbidden")
        elif kind == "download":
            if self.report["download_command_attempts"]:
                raise AssertionError("A second download command is forbidden")
        self.timeout()
        self.trace(event="write_intent", kind=kind, bytes=len(data))
        timeout = self.timeout()
        if kind in ("boot", "download"):
            self.report[kind + "_command_attempts"] += 1
        try:
            sent = self.usb.write(data, timeout)
        except Exception as error:
            if kind == "payload":
                self.report["payload_bytes_are_lower_bound"] = True
            raise BootTransportError("write " + kind + ": " + str(error)) from error
        if kind == "boot" and sent == len(data):
            self.report["boot_write_completed"] = True
        elif kind == "payload":
            self.report["payload_bytes_written"] += max(0, min(sent, len(data)))
        self.trace(event="write_return", kind=kind, requested=len(data), transferred=sent)
        if sent != len(data):
            raise BootTransportError("Incomplete " + kind + " write; not retried")

    def status(self):
        for _ in range(128):
            try:
                packet = bytes(self.usb.read(64, self.timeout()))
            except Exception as error:
                raise BootTransportError("read " + self.phase + ": " + str(error)) from error
            if not 4 <= len(packet) <= 64:
                raise BootTransportError("Invalid Fastboot status packet length")
            kind, body = packet[:4], packet[4:].rstrip(b"\x00")
            self.trace(event="status", phase=self.phase, kind=kind.decode("ascii", "replace"),
                       payload=body.decode("ascii", "replace"))
            if kind == b"INFO":
                continue  # receive-only, never resend the command
            if kind not in (b"OKAY", b"DATA", b"FAIL"):
                raise BootTransportError("Unknown Fastboot status")
            return kind, body
        raise BootTransportError("Too many INFO packets")

    def run(self, image):
        try:
            if not 4096 <= len(image) <= MAX_IMAGE:
                raise BootTransportError("Image size outside the RAM boot limit")
            self.write(("download:%08x" % len(image)).encode("ascii"), "download")
            kind, body = self.status()
            if kind != b"DATA" or not re.fullmatch(rb"[0-9a-fA-F]{8}", body) or int(body, 16) != len(image):
                raise BootTransportError("Download was not accepted with the exact image length")
            self.phase = "payload"
            for offset in range(0, len(image), CHUNK):
                self.write(image[offset:offset + CHUNK], "payload")
            kind, _ = self.status()
            if kind != b"OKAY":
                raise BootTransportError("Payload completion was not acknowledged")
            self.report["payload_acknowledged"] = True
            self.phase = "boot"
            self.write(b"boot", "boot")
            kind, _ = self.status()
            if kind != b"OKAY":
                raise BootTransportError("Boot was not acknowledged")
            self.report["result"] = "BOOT_ACKNOWLEDGED"
        except BootTransportError as error:
            self.report["result"] = "BOOT_UNCERTAIN" if self.report["boot_command_attempts"] else "STOP_BEFORE_BOOT"
            self.report["error"] = str(error)
        except AssertionError:
            raise
        except Exception as error:
            self.report["result"] = "BOOT_UNCERTAIN" if self.report["boot_command_attempts"] else "STOP_BEFORE_BOOT"
            self.report["error"] = str(error)
        try:
            self.trace(event="terminal", **self.report)
        except Exception as error:
            self.report["trace_error"] = str(error)
        return dict(self.report)


class SonyUsb:
    def __init__(self):
        import libusb_package
        import usb.core
        import usb.util
        import usb.backend.libusb1 as backend
        self.util, self.device, self.claimed = usb.util, None, False
        lib = backend.get_backend(find_library=libusb_package.find_library)
        if lib is None:
            raise BootTransportError("libusb backend unavailable")
        devices = list(usb.core.find(find_all=True, idVendor=VID, idProduct=PID, backend=lib))
        if len(devices) != 1:
            raise BootTransportError("Need exactly one Sony 0fce:0dde; no interface opened")
        candidate = devices[0]
        try:
            identity = usb.util.get_string(candidate, candidate.iSerialNumber)
            if identity != SERIAL:
                raise BootTransportError("Readable Sony serial must match BH905ND69G")
            config = candidate.get_active_configuration()
            interface = config[(0, 0)]
            endpoints = {ep.bEndpointAddress: ep for ep in interface}
            if not {0x01, 0x81} <= endpoints.keys() or any(
                    usb.util.endpoint_type(endpoints[ep].bmAttributes) != usb.util.ENDPOINT_TYPE_BULK for ep in (0x01, 0x81)):
                raise BootTransportError("Unexpected Sony Fastboot bulk interface")
            # Existing configuration only. Never reset, set_configuration or detach a driver.
            usb.util.claim_interface(candidate, 0)
            self.device, self.claimed = candidate, True
        except Exception:
            usb.util.dispose_resources(candidate)
            raise

    def write(self, data, timeout):
        return self.device.write(0x01, data, timeout=timeout)

    def read(self, size, timeout):
        return self.device.read(0x81, size, timeout=timeout)

    def close(self):
        if self.device is not None:
            try:
                if self.claimed:
                    self.util.release_interface(self.device, 0)
            finally:
                self.util.dispose_resources(self.device)
                self.device, self.claimed = None, False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--image", required=True)
    parser.add_argument("--sha256", required=True)
    parser.add_argument("--result")
    parser.add_argument("--live", action="store_true")
    args = parser.parse_args()
    image = load_image(args.image, args.sha256)
    sender_sha = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if not args.live:
        print(json.dumps({"offline": True, "image_sha256": args.sha256, "image_bytes": len(image),
                          "sender_sha256": sender_sha, "usb_opened": False}))
        return 0
    if not args.result:
        parser.error("Live operation requires a result file")
    output = Path(args.result).resolve()
    if Path(os.environ["TEMP"]).resolve() not in output.parents or not output.parent.is_dir() or output.exists():
        raise ValueError("Result must be a new file in the owned TEMP run directory")
    trace_path = output.with_suffix(".jsonl")
    if trace_path.exists():
        raise ValueError("Preserve previous wire records")
    def trace(**event):
        with trace_path.open("a", encoding="utf-8", newline="\n") as file:
            file.write(json.dumps({"time_monotonic": time.monotonic(), **event}) + "\n")
            file.flush()
            os.fsync(file.fileno())
    usb = None
    runner = None
    try:
        usb = SonyUsb()
        runner = BootOnce(usb, trace=trace)
        report = runner.run(image)
    except Exception as error:
        report = dict(runner.report) if runner is not None else {
            "serial": SERIAL, "boot_command_attempts": 0, "download_command_attempts": 0, "retries": 0}
        report.update(result="BOOT_UNCERTAIN" if report["boot_command_attempts"] else "REFUSE_BEFORE_TRANSFER", error=str(error))
        try:
            trace(event="refused", **report)
        except Exception as trace_error:
            report["trace_error"] = str(trace_error)
    finally:
        if usb is not None:
            try:
                usb.close()
            except Exception as error:
                report["release_error"] = str(error)
    report.update(image_sha256=args.sha256, sender_sha256=sender_sha)
    output.write_text(json.dumps(report, indent=2) + "\n", newline="\n")
    print(json.dumps(report), flush=True)
    return 0 if report["result"] == "BOOT_ACKNOWLEDGED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
