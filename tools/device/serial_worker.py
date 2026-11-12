"""One process owns CDC read/write; the supervisor can end a stuck driver call."""
import argparse
from pathlib import Path
import json
import time

def main():
    from serial import Serial
    from serial.tools import list_ports
    from policy import serial_candidates
    ap = argparse.ArgumentParser()
    ap.add_argument("--directory", required=True)
    ap.add_argument("--token", required=True)
    ap.add_argument("--attach-seconds", type=float, default=90)
    args = ap.parse_args()
    root = Path(args.directory)
    event_file = root / "serial-events.jsonl"
    command_file = root / "serial-commands.jsonl"
    def event(kind, **data):
        with event_file.open("a", encoding="utf-8") as f:
            f.write(json.dumps({"time": time.time(), "event": kind, **data}) + "\n"); f.flush()
    port = None
    end = time.monotonic() + args.attach_seconds
    while port is None and time.monotonic() < end:
        candidates = [p for p in list_ports.comports() if p.vid == 0x1209 and p.pid == 1]
        if len(candidates) > 1:
            event("refuse", reason="multiple_debug_devices"); return 3
        if candidates:
            try:
                p = serial_candidates(candidates)
                port = Serial(p.device, baudrate=115200, timeout=.15, write_timeout=1)
                port.dtr = True
                event("attached", port=p.device, vid=p.vid, pid=p.pid)
            except Exception as e:
                event("attach_failed", reason=str(e)); time.sleep(.3)
        else: time.sleep(.2)
    if port is None: event("timeout", reason="no_debug_cdc"); return 2
    cursor = 0
    pending = ""
    with port, (root / "serial.raw.log").open("wb") as log:
        # Current-boot ring is replayed first; LIVE validates that this CDC is the expected artifact.
        for cmd in ("replay", f"loop {args.token}", "status"):
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
                        # The kernel queues this fixed upward gesture on the RMI scan thread.
                        allowed = {"frame", "input", "sample", "status", "swipe", f"loop {args.token}", f"fastboot {args.token}"}
                        if cmd not in allowed: event("refuse", reason="command_not_allowed"); return 5
                        port.write((cmd + "\n").encode("ascii")); port.flush(); event("tx", command=cmd)
            except Exception as e:
                event("disconnected", reason=str(e)); return 0

if __name__ == "__main__":
    raise SystemExit(main())
