"""Wait for external boot tools to retire, then dispatch one reviewed trial.

The wait itself opens no phone/serial endpoint, terminates no external process,
and never retries a dispatched package. run_trial retains every original gate.
"""

import argparse
import json
import os
from pathlib import Path
import sys
import time
import subprocess

from controller import HERE, PATCH, write_json
from policy import Refuse
from procguard import LiveLock, OwnedProcess, active_tool_processes, run as run_process


def idle_transition(now, conflicts, idle_since, stable_seconds):
    if conflicts:
        return None, False
    if idle_since is None:
        idle_since = now
    return idle_since, now - idle_since >= stable_seconds


def validate_request(request, wait_directory):
    directory = Path(wait_directory).resolve()
    temp = Path(os.environ["TEMP"]).resolve()
    if not directory.is_relative_to(temp) or directory.exists():
        raise Refuse("Use a new waiting directory under Windows TEMP")
    if not isinstance(request, dict) or request.get("live") is not True:
        raise Refuse("The waiting request must explicitly contain one live trial")
    allowed = {
        "bundle", "directory", "private_evidence", "owner_evidence", "ready_evidence",
        "rpm_evidence", "cpu_evidence", "gles_evidence", "native_evidence", "png_evidence", "live",
        "wait_before_guard",
    }
    if set(request) - allowed or not {"bundle", "directory", "private_evidence", "live"} <= set(request):
        raise Refuse("Waiting request has unsupported or incomplete trial arguments")
    bundle = Path(request["bundle"]).resolve()
    trial_directory = Path(request["directory"]).resolve()
    if not bundle.is_relative_to(PATCH / "build") or not (bundle / "artifacts.json").is_file():
        raise Refuse("Waiting requires an existing reviewed formal trial bundle")
    if not trial_directory.is_relative_to(temp) or trial_directory.exists() or trial_directory == directory:
        raise Refuse("The actual trial needs its own new TEMP directory")
    return directory, bundle, trial_directory


def wait_and_dispatch(request, wait_directory, *, stable_seconds=30, limit_seconds=1800):
    if not 30 <= stable_seconds <= 120 or not stable_seconds <= limit_seconds <= 7200:
        raise Refuse("Waiting duration exceeds the reviewed bound")
    directory, bundle, trial_directory = validate_request(request, wait_directory)
    directory.mkdir(parents=True)
    write_json(directory / "request.json", request)
    # This mutex covers waiting helpers only. run_trial acquires the original
    # Sony device mutex independently and still rejects any intervening race.
    lock = LiveLock(directory / "waiting.lock", name="Local\\lilac-wait-BH905ND69G")
    report = dict(serial="BH905ND69G", opens_phone_or_serial=False, terminates_external_process=False,
                  dispatched=False, retries=0, stable_seconds=stable_seconds,
                  limit_seconds=limit_seconds)
    try:
        start = time.monotonic()
        idle_since = None
        previous = None
        while time.monotonic() - start < limit_seconds:
            if (directory / "STOP").exists():
                raise Refuse("Waiting cancelled before trial dispatch")
            now = time.monotonic()
            conflicts = active_tool_processes()
            current = tuple(sorted((x["pid"], x["exe"]) for x in conflicts))
            if current != previous:
                with (directory / "events.jsonl").open("a", encoding="utf-8", newline="\n") as stream:
                    stream.write(json.dumps(dict(elapsed_seconds=round(now - start, 3),
                                                 conflicting_tools=conflicts)) + "\n")
                print(json.dumps(dict(waiting=True, conflicting_tools=conflicts)), flush=True)
                previous = current
            idle_since, ready = idle_transition(now, conflicts, idle_since, stable_seconds)
            if ready:
                # The slower legacy-flow query runs after the fast name scan
                # is stable. Recheck the fast scan before allowing one dispatch.
                code, out, err = run_process(
                    ["powershell", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",
                     "-File", HERE / "preflight_processes.ps1"], cwd=PATCH, timeout=30)
                with (directory / "legacy-preflight.log").open("a", encoding="utf-8") as stream:
                    stream.write(out + err)
                if code:
                    raise Refuse("Legacy-flow preflight failed before trial dispatch")
                if json.loads(out) or active_tool_processes():
                    idle_since = None
                    continue
                if trial_directory.exists():
                    raise Refuse("Trial directory appeared while waiting; no replay permitted")
                report.update(dispatched=True, wait_seconds=time.monotonic() - start,
                              continuous_tool_idle_seconds=time.monotonic() - idle_since)
                write_json(directory / "result.json", report)
                argv = [sys.executable, "-B", str(HERE / "run_trial.py")]
                for key, value in request.items():
                    if key == "live":
                        argv.append("--live")
                    else:
                        argv += ["--" + key.replace("_", "-"), str(value)]
                write_json(directory / "dispatch.json", dict(argv=argv, bundle=str(bundle),
                                                             max_dispatches=1, failure_never_retried=True))
                print(json.dumps(dict(dispatching_once=True, bundle=str(bundle))), flush=True)
                # Popen's argument-list quoting preserves Windows paths with
                # spaces. The helper owns one controller child, never serial.
                # Keep the waiting mutex until that child has fully exited.
                with (directory / "controller.log").open("xb") as output:
                    with OwnedProcess(argv, cwd=PATCH, stdout=output, stderr=subprocess.STDOUT) as child:
                        write_json(directory / "controller.json", dict(
                            pid=child.process.pid, argv=argv, serial_owner_is_child_only=True))
                        while child.process.poll() is None:
                            time.sleep(1)
                        code = child.process.returncode
                report.update(result="CONTROLLER_EXITED", controller_exit_code=code,
                              trial_directory=str(trial_directory), child_fully_exited=True)
                write_json(directory / "result.json", report)
                print(json.dumps(dict(controller_exited=True, exit_code=code,
                                      trial_directory=str(trial_directory))), flush=True)
                return code
            write_json(directory / "progress.json", dict(
                waiting=True, elapsed_seconds=round(now - start, 3), conflicting_tools=conflicts,
                idle_seconds=round(now - idle_since, 3) if idle_since is not None else 0))
            time.sleep(1)
        raise Refuse("External tool idle window did not occur; package not dispatched")
    except BaseException as error:
        report.update(result="STOP_BEFORE_DISPATCH" if not report["dispatched"] else "DISPATCH_FAILED",
                      error=str(error))
        write_json(directory / "result.json", report)
        raise
    finally:
        lock.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--request", required=True, help="Explicit JSON run_trial arguments")
    parser.add_argument("--directory", required=True, help="New TEMP waiting journal")
    parser.add_argument("--stable-seconds", type=int, default=30)
    parser.add_argument("--limit-seconds", type=int, default=1800)
    args = parser.parse_args()
    request = json.loads(Path(args.request).read_bytes())
    raise SystemExit(wait_and_dispatch(request, args.directory, stable_seconds=args.stable_seconds,
                                       limit_seconds=args.limit_seconds))
