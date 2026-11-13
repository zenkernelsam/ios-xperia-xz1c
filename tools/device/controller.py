"""Bounded lilac test loop. Default validation/replay is entirely offline."""
import argparse
import csv
import shutil
from dataclasses import dataclass
from datetime import datetime, timezone, timedelta
from pathlib import Path
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
sys.dont_write_bytecode = True
import tempfile
import time

from policy import Decision, LogPolicy, Refuse, SERIAL, unique_fastboot, FastbootStability
from procguard import OwnedProcess, LiveLock, active_tool_processes, run as run_process
from sony_usb_guard import target_fastboot_lines

HERE = Path(__file__).resolve().parent
PATCH = HERE.parent.parent
IOS = PATCH
SHIM_SHA = "e1369bff6498d6f03f4ae29559bb99ebc7f3f9a7eee71a0f0d6a9e68ff73d7c3"

def write_json(path, data):
    path = Path(path); pending = path.with_name(path.name + ".new")
    with pending.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2); f.write("\n"); f.flush(); os.fsync(f.fileno())
    pending.replace(path)

@dataclass
class Artifact:
    data: dict
    @property
    def image(self): return Path(self.data["image"]).resolve()
    @property
    def token(self): return self.data["token"]
    @property
    def watchdog(self): return self.data["watchdog_seconds"]
    def validate(self):
        p = self.image
        if not any(root.resolve() in p.parents for root in (PATCH, IOS)):
            raise Refuse("Image must stay inside the two Xperia project folders")
        if p.suffix != ".img" or p.stat().st_size > 64 * 1024 * 1024:
            raise Refuse("Boot image must be .img and <=64MiB")
        raw = p.read_bytes()
        if hashlib.sha256(raw).hexdigest() != self.data["sha256"]: raise Refuse("Image SHA mismatch")
        if raw[:8] != b"ANDROID!" or struct.unpack_from("<I", raw, 0x24)[0] != 4096 or struct.unpack_from("<I", raw, 0x18)[0]:
            raise Refuse("Not the approved Android v0/shim image format")
        ks, rs = struct.unpack_from("<I", raw, 8)[0], struct.unpack_from("<I", raw, 16)[0]
        off = 4096 + (ks + 4095) // 4096 * 4096
        payload = raw[off:off + rs]
        if hashlib.sha256(raw[4096:4096 + ks]).hexdigest() != SHIM_SHA:
            raise Refuse("Shim differs from the verified PON06 v648 shim")
        if payload[:8] != b"SWEETXNU" or len(payload) != rs: raise Refuse("Incomplete payload")
        cmd = payload[72:680].split(b"\0", 1)[0].decode("ascii")
        if cmd != self.data["cmdline"]: raise Refuse("Embedded command differs from manifest")
        words = cmd.split(); options = {}
        for word in words:
            if "=" in word:
                key, value = word.split("=", 1)
                if key in options: raise Refuse(f"Duplicate boot argument {key}")
                options[key] = value
        if not re.fullmatch(r"[0-9a-f]{16}", self.token) or options.get("sweetloop") != self.token:
            raise Refuse("Missing artifact-associated token")
        required = {"serial": "3", "cpus": "8", "lilac_usbdbg": "1", "shimponly": "1"}
        if any(options.get(k) != v for k, v in required.items()): raise Refuse("Required debug/route arguments missing")
        if int(options.get("sweetar", "0")) != self.watchdog or not 60 <= self.watchdog <= 10800:
            raise Refuse("Invalid watchdog deadline")
        for key in ("sweetnoar", "sweetnoreboot", "sweetnohostreboot"):
            if key in words or options.get(key, "0") != "0": raise Refuse(f"Suppression flag {key}")
        for marker in (b"SWEET LOOP LIVE proto=1", b"SWEET LOOP ACK FASTBOOT", b"SWEET LOOP RESET pon=0x06"):
            if marker not in payload: raise Refuse("RPC or PON06 reboot marker absent")
        if "boot_sender_sha256" in self.data:
            if self.data.get("boot_sender") != "sony-boot-once" or \
                    hashlib.sha256((HERE / "boot_once.py").read_bytes()).hexdigest() != self.data["boot_sender_sha256"]:
                raise Refuse("Boot sender differs from the artifact's audited transport revision")
        return self
    def experiment(self):
        # Changing only a token or compiler timestamp is not a new hypothesis.
        args = tuple(sorted(x for x in self.data["cmdline"].split() if not x.startswith("sweetloop=")))
        return self.data["source_revision"], args, self.data.get("boot_sender_sha256")

class Journal:
    def __init__(self, root):
        self.root = Path(root); self.root.mkdir(exist_ok=True)
        self.rows = []
    def event(self, state, **data):
        row = {"time": datetime.now(timezone(timedelta(hours=8))).isoformat(), "state": state, **data}
        with (self.root / "events.jsonl").open("a", encoding="utf-8") as f:
            f.write(json.dumps(row) + "\n"); f.flush(); os.fsync(f.fileno())
    def result(self, row):
        self.rows.append(row); write_json(self.root / "results.json", self.rows)
        fields = ["cycle", "image_sha", "source_revision", "decision", "reason", "result", "boot_dispatches",
                  "reset_requests", "fastboot_returned", "reset_ack", "reset_to_fastboot_seconds", "simulated",
                  "boot_command_attempts", "download_command_attempts", "boot_transfer_result",
                  "return_log_complete", "final_reset_marker_seen"]
        with (self.root / "results.csv").open("w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=fields); writer.writeheader()
            for item in self.rows:
                record = {k: item.get(k, "") for k in fields}
                record.update(image_sha=item["artifact"]["sha256"], source_revision=item["artifact"]["source_revision"])
                wire = item.get("boot_wire") or {}
                record.update(boot_command_attempts=wire.get("boot_command_attempts", ""),
                    download_command_attempts=wire.get("download_command_attempts", ""),
                    boot_transfer_result=wire.get("result", ""))
                writer.writerow(record)

class LiveOps:
    simulated = False
    def __init__(self, journal):
        self.journal = journal; self.capture_process = None; self.worker_err = None
        self.fastboot = Path(os.environ.get("LILAC_FASTBOOT", shutil.which("fastboot") or ""))
        self.bash = Path(os.environ.get("LILAC_BASH", shutil.which("bash") or ""))
        self.env = dict(os.environ, MSYS_NO_PATHCONV="1", PYTHONUNBUFFERED="1", PYTHONDONTWRITEBYTECODE="1")
        self.directory = None; self.offset = 0; self.pending = ""; self.transmitted = 0
        self.deadline = None
        self.boot_attempted = False
        self.boot_wire = None
        self.boot_wire_error = None
    def check_stop(self):
        if (self.journal.root / "STOP").exists(): raise Refuse("Operator stop requested")
        if self.deadline is not None and time.monotonic() >= self.deadline: raise Refuse("Whole-run time budget reached")
    def command(self, argv, timeout=15, label="command"):
        self.check_stop()
        if self.deadline is not None: timeout = min(timeout, max(.1, self.deadline - time.monotonic()))
        code, out, err = run_process(argv, cwd=PATCH, timeout=timeout, env=self.env)
        self.journal.event(label, argv=[str(x) for x in argv], code=code)
        n = len(list(self.journal.root.glob("command-*.log")))
        (self.journal.root / f"command-{n:03d}.log").write_text(out + "\n" + err, encoding="utf-8")
        if code or re.search(r"\bFAILED\b|ERROR:", out + err, re.I): raise Refuse(f"{label} failed: {(out+err)[-600:]}")
        return out + err
    def assert_fastboot(self):
        text = self.command([self.fastboot, "devices"], 8, "inventory")
        unique_fastboot(text)
        text = self.command([self.fastboot, "-s", SERIAL, "getvar", "version-bootloader"], 8, "health")
        if "version-bootloader:" not in text: raise Refuse("Fastboot endpoint unhealthy; physical replug may be required")
    def patch(self):
        from sony_usb_guard import validate_inputs
        validate_inputs(PATCH / "assets/device-tools")
        if active_tool_processes(): raise Refuse("Another fastboot/xperable process is running")
        text = self.command(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", HERE / "preflight_processes.ps1"], 12, "host_process_preflight")
        if json.loads(text): raise Refuse("A legacy boot/capture loop is running; refusing to interleave")
        text = self.command([self.bash, HERE / "guarded_xperable.sh", "-B", "-U", "-s", "0xf49880", "-x", "-4", "-6"], 45, "patch")
        if text.count("patched successfully") != 2: raise Refuse("ABL patch was not exactly 2/2")
    def boot(self, artifact, directory):
        from serial.tools import list_ports
        self.directory = directory; self.offset = 0; self.pending = ""; self.transmitted = 0
        self.boot_attempted = False
        self.boot_wire = None
        self.boot_wire_error = None
        # A debug CDC already present cannot be associated with this pending boot.
        if any(p.vid == 0x1209 and p.pid == 1 for p in list_ports.comports()):
            raise Refuse("Pre-existing debug CDC; refusing ambiguous handoff")
        if active_tool_processes(): raise Refuse("Another fastboot/xperable process is running before boot")
        self.worker_err = (directory / "serial-worker.stderr").open("wb")
        self.capture_process = OwnedProcess([sys.executable, HERE / "serial_worker.py", "--directory", directory,
            "--token", artifact.token, "--attach-seconds", "90"], cwd=PATCH, env=self.env,
            stdout=subprocess.DEVNULL, stderr=self.worker_err)
        # One dispatch only. A timeout/nonzero result is ambiguous, never an invitation to retry.
        artifact.validate()
        sender = HERE / "boot_once.py"
        sender_sha = hashlib.sha256(sender.read_bytes()).hexdigest()
        wire_path = directory / "boot-wire.json"
        self.journal.event("BOOT_SENDER", sender="sony-boot-once", source_sha256=sender_sha,
            configured_command_retries=0, wire_report=str(wire_path))
        self.boot_attempted = True
        try:
            self.command([sys.executable, sender, "--image", artifact.image, "--sha256", artifact.data["sha256"],
                "--result", wire_path, "--live"], 45, "boot_once")
        finally:
            if wire_path.exists():
                try:
                    report = json.loads(wire_path.read_text())
                    if not isinstance(report, dict):
                        raise ValueError("Wire report must be an object")
                    if report.get("serial") != SERIAL or report.get("image_sha256") != artifact.data["sha256"] or \
                            report.get("sender_sha256") != sender_sha or report.get("retries") != 0 or \
                            report.get("boot_command_attempts") not in (0, 1) or report.get("download_command_attempts") not in (0, 1):
                        raise ValueError("Wire report identity/count mismatch")
                    self.boot_wire = report
                    self.journal.event("BOOT_WIRE_REPORT", **report)
                except (OSError, ValueError, TypeError) as error:
                    self.boot_wire_error = str(error)
                    self.journal.event("BOOT_WIRE_REPORT_INVALID", error=str(error))
            else:
                self.boot_wire_error = "Wire result unavailable; command outcome remains uncertain"
                self.journal.event("BOOT_WIRE_REPORT_UNAVAILABLE")
    def lines(self):
        path = self.directory / "serial.raw.log"
        if not path.exists(): return []
        with path.open("rb") as f:
            f.seek(self.offset); data = f.read(256 * 1024); self.offset = f.tell()
        self.pending += data.decode("ascii", "replace")
        if len(self.pending) > 1024 * 1024: raise Refuse("Unbounded serial line")
        parts = self.pending.split("\n"); self.pending = parts.pop()
        return [p.rstrip("\r") for p in parts]
    def observe(self, artifact, step):
        policy = LogPolicy(artifact.token, step.get("goals", []), step.get("stable_seconds", 10))
        start = time.monotonic()
        while time.monotonic() - start < step["observe_seconds"]:
            self.check_stop(); now = time.monotonic() - start
            for line in self.lines(): policy.feed(line, now)
            if policy.actual_watchdog is not None and policy.actual_watchdog != artifact.watchdog:
                return Decision("STOP", "WATCHDOG_CONFIG_MISMATCH"), policy.live
            decision = policy.decision(now)
            if decision.kind != "WAIT": return decision, policy.live
            if self.capture_process.process.poll() is not None:
                return Decision("STOP", "SERIAL_WORKER_EXITED"), False
            time.sleep(.1)
        return Decision("STOP", "OBSERVATION_INCOMPLETE"), policy.live
    def camera(self, directory):
        self.command([sys.executable, HERE / "capture.py", directory], 35, "camera")
        data = json.loads((directory / "capture.json").read_text())
        if not data.get("frames"): raise Refuse("No camera evidence")
        return data
    def request_reset(self, artifact):
        self.check_stop()
        if self.transmitted: raise Refuse("Reset already requested in this cycle")
        if self.capture_process.process.poll() is not None: raise Refuse("Serial worker exited before reset")
        with (self.directory / "serial-commands.jsonl").open("a", encoding="utf-8") as f:
            f.write(json.dumps({"command": f"fastboot {artifact.token}"}) + "\n"); f.flush(); os.fsync(f.fileno())
        self.transmitted = 1
    def wait_return(self, artifact, seconds):
        ack = False; start = time.monotonic(); last_probe = 0; stability=FastbootStability(30)
        while time.monotonic() - start < seconds:
            self.check_stop()
            for line in self.lines():
                if f"SWEET LOOP ACK FASTBOOT token={artifact.token}" in line: ack = True
                if "SWEET LOOP RESET_RETURNED" in line or "SWEET LOOP REJECT" in line:
                    return False, ack, time.monotonic() - start
            if time.monotonic() - last_probe >= 1:
                last_probe = time.monotonic()
                try:
                    text = self.command([self.fastboot, "devices"], 5, "return_inventory")
                    # Another phone remaining in fastboot says nothing about
                    # Sony returning. Validate target rows before health probing.
                    try:
                        present = bool(target_fastboot_lines(text))
                    except RuntimeError as error:
                        raise Refuse("Fastboot inventory must contain one valid Sony row: " + str(error)) from error
                    if present:
                        unique_fastboot(text); self.assert_fastboot()
                        for line in self.lines():
                            if f"SWEET LOOP ACK FASTBOOT token={artifact.token}" in line: ack = True
                        if stability.feed(time.monotonic()-start,True)=="STABLE":
                            return True, ack, time.monotonic()-start
                    elif stability.feed(time.monotonic()-start,False)=="LOST":
                        self.journal.event("FASTBOOT_UNSTABLE", first_seen=stability.since)
                        return False,ack,time.monotonic()-start
                except Refuse as e:
                    if "inventory must" in str(e): raise
                    if stability.since is not None: return False,ack,time.monotonic()-start
                except subprocess.TimeoutExpired:
                    if stability.since is not None: return False,ack,time.monotonic()-start
            time.sleep(.1)
        return False, ack, time.monotonic() - start
    def close(self):
        if self.capture_process: self.capture_process.close(); self.capture_process = None
        if self.worker_err: self.worker_err.close(); self.worker_err = None
    def produce(self, argv, request, output):
        self.command([*argv, "--request", request, "--out", output], 1800, "producer")
        return Artifact(json.loads(output.read_text())).validate()

class ReplayOps:
    simulated = True
    def __init__(self, scenario, journal): self.scenario = scenario; self.journal = journal; self.calls = []; self.n = -1
    def check_stop(self): pass
    def assert_fastboot(self): self.calls.append("health")
    def patch(self): self.calls.append("patch_2of2")
    def boot(self, artifact, directory): self.n += 1; self.calls.append("boot_once"); self.directory = directory; self.boot_attempted = True
    def observe(self, artifact, step):
        policy = LogPolicy(artifact.token, step.get("goals", []), step.get("stable_seconds", 10))
        frames = self.scenario["cycles"][min(self.n, len(self.scenario["cycles"]) - 1)]
        lines = []
        for entry in frames["log"]:
            line = entry["line"].replace("{token}", artifact.token); lines.append(line)
            policy.feed(line, entry["t"])
        (self.directory / "serial.raw.log").write_text('\n'.join(lines) + '\n')
        if policy.actual_watchdog is not None and policy.actual_watchdog != artifact.watchdog:
            return Decision("STOP", "WATCHDOG_CONFIG_MISMATCH"), policy.live
        return policy.decision(frames["end"]), policy.live
    def camera(self, directory):
        self.calls.append("camera_mjpeg")
        directory.mkdir(exist_ok=True); write_json(directory / "capture.json", {"simulated": True, "frames": ["synthetic"]})
        return {"simulated": True, "frames": ["synthetic"]}
    def request_reset(self, artifact): self.calls.append("reset_once")
    def wait_return(self, artifact, seconds):
        cycle = self.scenario["cycles"][min(self.n, len(self.scenario["cycles"]) - 1)]
        self.calls.append("wait_healthy_fastboot")
        stability=FastbootStability(30)
        timeline=cycle.get("return_timeline",[{"t":2,"present":True},{"t":32,"present":cycle.get("returned",True)}])
        state="WAIT"; elapsed=0
        for item in timeline:
            elapsed=item["t"]; state=stability.feed(elapsed,item["present"],item.get("healthy",True))
            if state=="LOST": break
        return state=="STABLE",cycle.get("ack",True),elapsed
    def close(self): pass

def validate_plan(plan):
    if plan.get("target_serial") != SERIAL or plan.get("pon_route") != 6: raise Refuse("Serial/PON policy mismatch")
    maximum = plan.get("max_cycles", 1)
    if maximum != 1 or not 1 <= plan.get("budget_seconds", 480) <= 28800:
        raise Refuse("Invalid bounded cycle/time budget")
    if not plan.get("steps"): raise Refuse("No experiment steps")
    for step in plan["steps"]:
        if not 1 <= step.get("observe_seconds", 0) <= 14400 or not 1 <= step.get("return_seconds", 60) <= 120:
            raise Refuse("Invalid observation/return window")
        for regex in step.get("goals", []): re.compile(regex)
        if step.get("return_evidence") != "final-drain":
            raise Refuse("Current device plans require complete final-drain evidence")
        for key in ("on_pass", "on_failure"):
            if step.get(key, "stop") != "stop": raise Refuse("Current device plans must stop after one result")
    return maximum

def execute(plan, artifacts, ops, journal):
    maximum = validate_plan(plan)
    started = time.monotonic(); index = 0; produced = None
    if not ops.simulated: ops.deadline = started + plan["budget_seconds"]
    for cycle in range(maximum):
        ops.check_stop()
        if time.monotonic() - started >= plan["budget_seconds"]: break
        step = plan["steps"][min(index, len(plan["steps"]) - 1)]
        artifact = (produced or Artifact(artifacts[step["artifact_id"]])).validate(); produced = None
        directory = journal.root / f"cycle-{cycle + 1:02d}"; directory.mkdir()
        row = {"cycle": cycle + 1, "artifact": artifact.data, "simulated": ops.simulated,
               "boot_dispatches": 0, "reset_requests": 0, "fastboot_returned": False}
        if hasattr(ops, "boot_wire"):
            ops.boot_wire = None
            ops.boot_wire_error = None
        try:
            journal.event("VALIDATED", cycle=cycle + 1, sha256=artifact.data["sha256"])
            ops.assert_fastboot(); journal.event("PATCHING", cycle=cycle + 1); ops.patch()
            ops.check_stop(); ops.assert_fastboot(); journal.event("BOOTING", cycle=cycle + 1)
            ops.boot(artifact, directory); row["boot_dispatches"] = 1
            if getattr(ops, "boot_wire", None) is not None:
                row["boot_wire"] = ops.boot_wire
            if getattr(ops, "boot_wire_error", None):
                row["boot_wire_error"] = ops.boot_wire_error
            journal.event("OBSERVING", cycle=cycle + 1)
            decision, live = ops.observe(artifact, step); row.update(decision=decision.kind, reason=decision.reason, live=live)
            if decision.kind == "WAIT": row.update(decision="STOP", reason="OBSERVATION_INCOMPLETE")
            write_json(directory / "decision.json", row)
            journal.event("EVIDENCE", cycle=cycle + 1, reason=row["reason"])
            if plan.get("camera_required", True):
                try:
                    row["camera"] = ops.camera(directory / "camera")
                except (Refuse, subprocess.TimeoutExpired, OSError) as e:
                    # Missing evidence stops further experiments, while a validated live
                    # session can still use its normal one-shot return protocol.
                    row.update(decision="STOP", reason="CAMERA_EVIDENCE_MISSING", camera_error=str(e))
                    write_json(directory / "decision.json", row)
                    journal.event("CAMERA_UNAVAILABLE", cycle=cycle + 1, error=str(e))
            # Flush decision/evidence before control. No nonce-validated LIVE means no reset command.
            raw = directory / "serial.raw.log"
            if raw.exists():
                prefix = raw.read_bytes(); (directory / "checkpoint.serial.log").write_bytes(prefix)
                row["checkpoint_log_bytes"] = len(prefix)
                row["checkpoint_log_sha256"] = hashlib.sha256(prefix).hexdigest()
            write_json(directory / "checkpoint.json", row)
            if not live:
                row["result"] = "STOP_NO_LIVE_SESSION"; journal.result(row); break
            journal.event("RESET_REQUESTED", cycle=cycle + 1, token=artifact.token)
            ops.request_reset(artifact); row["reset_requests"] = 1
            returned, ack, elapsed = ops.wait_return(artifact, step.get("return_seconds", 60))
            row.update(fastboot_returned=returned, reset_ack=ack, reset_to_fastboot_seconds=elapsed)
            ops.close()
            raw = directory / "serial.raw.log"
            if raw.exists(): row["log_sha256"] = hashlib.sha256(raw.read_bytes()).hexdigest()
            if not returned or not ack:
                row["result"] = "STOP_RESET_UNCONFIRMED"; journal.result(row); break
            journal.event("FASTBOOT_HEALTHY", cycle=cycle + 1)
            if step.get("return_evidence", "legacy") != "legacy":
                from return_evidence import inspect_return
                evidence = inspect_return(raw.read_bytes() if raw.exists() else b"", artifact.token,
                    row.get("checkpoint_log_bytes", 0), require_drain=step["return_evidence"] == "final-drain")
                row.update(return_log_complete=evidence["complete"],
                    final_reset_marker_seen=evidence["final_reset_marker_seen"], return_log_evidence=evidence)
                write_json(directory / "return-evidence.json", evidence)
                journal.event("RETURN_LOG_EVIDENCE", cycle=cycle + 1, **evidence)
                if not evidence["complete"]:
                    row.update(result="STOP_RETURN_EVIDENCE_MISSING", reason=evidence["reason"])
                    journal.result(row); break
            row["result"] = row["decision"]; journal.result(row)
            if row["decision"] == "STOP": break  # RPM/nonrecoverable states never retry.
            action = step.get("on_pass" if row["decision"] == "PASS" else "on_failure", "stop")
            if action == "stop": break
            if action == "next":
                if index + 1 >= len(plan["steps"]): break
                index += 1
            elif action == "repeat":
                if row["decision"] != "PASS": raise Refuse("Repeating a failed unchanged experiment is forbidden")
            elif action == "produce":
                request = directory / "revision-request.json"; write_json(request, row)
                output = directory / "next-artifact.json"
                journal.event("BUILDING_NEXT", cycle=cycle + 1)
                produced = ops.produce(plan["producer_argv"], request, output)
                if produced.experiment() == artifact.experiment(): raise Refuse("Producer changed no source or experiment argument")
            else: raise Refuse("Unknown next action")
        except BaseException as e:
            row["boot_dispatches"] = int(getattr(ops, "boot_attempted", row["boot_dispatches"]))
            row.update(result="STOP_ERROR", error=str(e)); journal.result(row); raise
        finally: ops.close()
    return journal.rows

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--plan", required=True)
    ap.add_argument("--artifacts", required=True)
    ap.add_argument("--live", action="store_true")
    ap.add_argument("--replay")
    args = ap.parse_args()
    if args.live and args.replay: ap.error("Choose live or replay")
    plan = json.loads(Path(args.plan).read_text()); catalog = json.loads(Path(args.artifacts).read_text())
    validate_plan(plan)
    if catalog.get("target_serial") != SERIAL or catalog.get("pon_route") != 6: raise Refuse("Artifact catalog policy mismatch")
    artifacts = {a["id"]: a for a in catalog["artifacts"]}
    for step in plan["steps"]: Artifact(artifacts[step["artifact_id"]]).validate()
    if not args.live and not args.replay:
        print("OFFLINE VALIDATION OK. No ports, camera, fastboot, or boot commands opened."); return 0
    root = Path(tempfile.mkdtemp(prefix="lilac-live-" if args.live else "lilac-replay-", dir=os.environ["TEMP"]))
    journal = Journal(root)
    # OS mutex/flock releases automatically if a controller dies; the JSON is only an ownership record.
    lock_path = Path(os.environ["TEMP"]) / "lilac-BH905ND69G-live.lock"
    lock = None
    if args.live:
        lock = LiveLock(lock_path)
        write_json(root / "owner.json", {"pid": os.getpid(), "run": str(root)})
    ops = LiveOps(journal) if args.live else ReplayOps(json.loads(Path(args.replay).read_text()), journal)
    try:
        rows = execute(plan, artifacts, ops, journal)
        print(json.dumps({"simulated": not args.live, "directory": str(root), "results": rows}, indent=2))
    finally:
        ops.close()
        if lock: lock.close()
    return 0 if rows and rows[-1]["result"] == "PASS" else 1

if __name__ == "__main__":
    try: raise SystemExit(main())
    except (Refuse, KeyboardInterrupt, subprocess.TimeoutExpired) as e:
        print("STOP:", e, file=sys.stderr); raise SystemExit(2)
