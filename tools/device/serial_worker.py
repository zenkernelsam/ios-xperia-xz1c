"""One process owns CDC read/write; the supervisor can end a stuck driver call."""
import argparse
from pathlib import Path
import hashlib
import json
import re
import time

CAPTURE_REVISION = 1


def capture_binding(token, worker_sha256=None):
    if not re.fullmatch(r"[0-9a-f]{16}", token):
        raise ValueError("Invalid current-boot token")
    if worker_sha256 is None:
        worker_sha256 = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    if not re.fullmatch(r"[0-9a-f]{64}", worker_sha256):
        raise ValueError("Invalid serial worker source SHA")
    return dict(revision=CAPTURE_REVISION, worker_sha256=worker_sha256,
                startup="closed-dtr-single-stream", dtr_before_open=False,
                raw_log_open_before_dtr=True, replay_commands=0,
                startup_commands=[f"loop {token}", "status"])


def open_serial(serial_class, device):
    # Windows Serial.open() configures DTR and then purges RX/TX. Constructing
    # an already-open Serial with its default DTR=True loses current-boot bytes
    # before a sink exists. Keep the kernel transmitter stopped through opening.
    port = serial_class(port=None, baudrate=115200, timeout=.15,
                        write_timeout=1, dsrdtr=False, exclusive=True)
    try:
        port.dtr = False
        port.port = device
        port.open()
        return port
    except BaseException:
        try:
            port.close()
        except Exception:
            pass
        raise


def capture_stream(port, root, token, event):
    cursor = 0
    pending = ""
    command_file = root / "serial-commands.jsonl"
    try:
        with port, (root / "serial.raw.log").open("xb") as log:
            event("log_ready", file="serial.raw.log", offset=log.tell(), mode="xb")
            port.dtr = True
            event("stream_started", dtr=True, replay_commands=0)
            # A fresh CDC has a zero retained-log cursor. Continue that stream
            # once; replay while IN is pending can splice two startup prefixes.
            for cmd in (f"loop {token}", "status"):
                port.write((cmd + "\n").encode("ascii")); port.flush(); event("tx", command=cmd)
            while True:
                try:
                    data = port.read(max(1, min(port.in_waiting, 65536)))
                    if data:
                        log.write(data); log.flush()
                        if log.tell() > 128 * 1024 * 1024:
                            event("limit", reason="raw_log_exceeded_128MiB"); return 4
                    if command_file.exists():
                        with command_file.open("rb") as f:
                            f.seek(cursor); added = f.read(); cursor = f.tell()
                        pending += added.decode("utf-8")
                        while "\n" in pending:
                            line, pending = pending.split("\n", 1)
                            record = json.loads(line)
                            cmd = record["command"]
                            # The kernel queues this fixed gesture on the RMI scan thread.
                            allowed = {"frame", "input", "sample", "status", "swipe", f"fastboot {token}"}
                            if cmd not in allowed: event("refuse", reason="command_not_allowed"); return 5
                            port.write((cmd + "\n").encode("ascii")); port.flush(); event("tx", command=cmd)
                except Exception as e:
                    event("disconnected", reason=str(e)); return 0
    finally:
        # Also closes a successful DTR-low open if creating the raw log failed.
        port.close()
        event("closed")


def main(argv=None):
    from serial import Serial
    from serial.tools import list_ports
    from policy import serial_candidates
    ap = argparse.ArgumentParser()
    ap.add_argument("--directory", required=True)
    ap.add_argument("--token", required=True)
    ap.add_argument("--source-sha256", required=True)
    ap.add_argument("--attach-seconds", type=float, default=90)
    args = ap.parse_args(argv)
    root = Path(args.directory)
    event_file = root / "serial-events.jsonl"
    def event(kind, **data):
        with event_file.open("a", encoding="utf-8") as f:
            f.write(json.dumps({"time": time.time(), "event": kind, **data}) + "\n"); f.flush()
    binding = capture_binding(args.token)
    if args.source_sha256 != binding["worker_sha256"]:
        event("refuse", reason="serial_worker_source_changed"); return 6
    event("capture_source", binding=binding)
    port = None
    end = time.monotonic() + args.attach_seconds
    while port is None and time.monotonic() < end:
        candidates = [p for p in list_ports.comports() if p.vid == 0x1209 and p.pid == 1]
        if len(candidates) > 1:
            event("refuse", reason="multiple_debug_devices"); return 3
        if candidates:
            try:
                p = serial_candidates(candidates)
                port = open_serial(Serial, p.device)
                event("attached", port=p.device, vid=p.vid, pid=p.pid, dtr=False, exclusive=True)
            except Exception as e:
                event("attach_failed", reason=str(e)); time.sleep(.3)
        else: time.sleep(.2)
    if port is None: event("timeout", reason="no_debug_cdc"); return 2
    return capture_stream(port, root, args.token, event)

if __name__ == "__main__":
    raise SystemExit(main())
