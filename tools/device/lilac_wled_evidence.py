"""Verify one WLED snapshot inside an actual scanout-bounded trial."""

import argparse
import binascii
import hashlib
import json
import re
from pathlib import Path

from scanout_trial_evidence import verify as verify_scanout
from diagnostic_candidate import WLED_KERNEL_SHA, WLED_VERSION
from trial_evidence import verify as verify_trial


STATES = {
    0: "UNAVAILABLE",
    1: "CHANGED",
    2: "FAULT",
    3: "DISABLED",
    4: "PARTIAL",
    5: "ZERO_DUTY",
    6: "CONFIGURED",
}
SAMPLE_RECORD = re.compile(r"(SWEET WLED SAMPLE .+ read_only=1) wire_crc=([0-9a-f]{8})")
SAMPLE_FIELDS = re.compile(
    r"SWEET WLED SAMPLE token=([0-9a-f]{16}) id=(\d+) state=(\d+) fault=(\d+) "
    r"enabled=(\d+) sinks=(\d+) modulators=(\d+) duty0=(\d+) duty1=(\d+) read_only=1"
)


def inspect(directory, expected_artifact_sha256):
    root = Path(directory).resolve()
    rows = json.loads((root / "results.json").read_bytes())
    if len(rows) != 1:
        raise ValueError("WLED attribution requires a single recorded trial")
    artifact = rows[0]["artifact"]
    if (artifact["version"], artifact["kernel_sha256"], artifact["sha256"]) != (
        WLED_VERSION, WLED_KERNEL_SHA, expected_artifact_sha256
    ):
        raise ValueError("WLED report is not bound to the exact observation artifact")
    if artifact["trial_mode"] == "private":
        private = verify_trial(root, "private", WLED_KERNEL_SHA, WLED_VERSION)
        run = {**private, "raw_sha256": private["log_sha256"], "original_result": rows[0]["result"]}
    elif artifact["trial_mode"] == "gpu":
        run = verify_scanout(root)
    else:
        raise ValueError("Unexpected WLED observation stage")
    if run["image_sha256"] != expected_artifact_sha256:
        raise ValueError("WLED report is not bound to the exact observation artifact")
    raw = (root / "cycle-01/serial.raw.log").read_bytes()
    frames = run["screen"]["frames"]
    if len(frames) != 1 or frames[0]["id"] != 1:
        raise ValueError("WLED requires the complete first scanout frame")
    if hashlib.sha256(raw).hexdigest() != run["raw_sha256"]:
        raise ValueError("Raw log changed after trial verification")
    text = raw.decode("ascii", "replace")
    live = f"SWEET LOOP LIVE proto=1 token={run['token']}"
    ack = f"SWEET LOOP ACK FASTBOOT token={run['token']}"
    begin = f"SWEET SCREEN BEGIN token={run['token']} id=1 "
    end = f"SWEET SCREEN END token={run['token']} id=1 "
    if any(text.count(marker) != 1 for marker in (live, ack, begin, end)):
        raise ValueError("WLED snapshot lacks one complete current-frame boundary")
    start, stop = text.index(live), text.index(ack)
    if not start < text.index(begin) < text.index(end) < stop:
        raise ValueError("WLED observation is outside the completed scanout and current LIVE")
    sample_lines = [line for line in text[start:stop].splitlines() if "SWEET WLED SAMPLE" in line]
    if len(sample_lines) > 1:
        raise ValueError("Expected at most one opt-in WLED snapshot")
    if not sample_lines:
        return dict(token=run["token"], image_sha256=run["image_sha256"], scanout_raw_sha256=run["raw_sha256"],
                    sample={"status": "unavailable", "reason": "no_bounded_wled_record"},
                    same_live_as_complete_scanout=True, original_trial_result=run["original_result"],
                    pwm_current_luminosity_verified=False, optical_panel_verified=False, device_operations=0)
    if not text.index(end) < text.index(sample_lines[0]) < stop:
        raise ValueError("WLED snapshot must follow the completed frame before ACK")
    values = decode_sample(sample_lines[0], run["token"])
    return dict(token=run["token"], image_sha256=run["image_sha256"], scanout_raw_sha256=run["raw_sha256"],
                sample=values, same_live_as_complete_scanout=True, original_trial_result=run["original_result"],
                pwm_current_luminosity_verified=False, optical_panel_verified=False,
                device_operations=0)


def decode_sample(line, expected_token):
    wire = SAMPLE_RECORD.fullmatch(line)
    if wire is None or not wire[1].isascii() or binascii.crc32(wire[1].encode("ascii")) != int(wire[2], 16):
        raise ValueError("WLED record CRC differs")
    match = SAMPLE_FIELDS.fullmatch(wire[1])
    if match is None:
        raise ValueError("Malformed WLED snapshot")
    token, frame, state, fault, enabled, sinks, modulators, duty0, duty1 = match.groups()
    values = dict(token=token, frame_id=int(frame), state=int(state), state_name=STATES.get(int(state)),
                  faults=int(fault), module_enabled=int(enabled), sink_mask=int(sinks),
                  modulator_mask=int(modulators), duty=[int(duty0), int(duty1)])
    if token != expected_token or frame != "1" or int(state) not in STATES:
        raise ValueError("WLED snapshot identity/state differs")
    if (values["faults"] > 7 or values["module_enabled"] > 1 or values["sink_mask"] > 15
            or values["modulator_mask"] > 3 or max(values["duty"]) > 4095):
        raise ValueError("WLED register value exceeds the hardware contract")
    if values["state"] < 2:
        expected = values["state"] if not any(
            [values["faults"], values["module_enabled"], values["sink_mask"], values["modulator_mask"], *values["duty"]]
        ) else None
    elif values["faults"]:
        expected = 2
    elif not values["module_enabled"]:
        expected = 3
    elif values["sink_mask"] != 3 or values["modulator_mask"] != 3 or duty0 != duty1:
        expected = 4
    else:
        expected = 6 if values["duty"][0] else 5
    if values["state"] != expected:
        raise ValueError("WLED state disagrees with the reported registers")
    return values


def wait_for_sample(read_tail, worker_alive, check_stop, token, deadline, clock, sleep):
    pending = b""
    after_end = False
    seen = 0
    end = f"SWEET SCREEN END token={token} id=1 ".encode("ascii")
    while clock() < deadline:
        check_stop()
        chunk = read_tail()
        seen += len(chunk)
        if seen > 1024 * 1024:
            return "unavailable"
        lines = (pending + chunk).split(b"\n")
        pending = lines.pop()
        for line in lines:
            if line.startswith(end):
                after_end = True
            elif after_end and line.startswith(b"SWEET WLED SAMPLE "):
                try:
                    decode_sample(line.rstrip(b"\r").decode("ascii"), token)
                except ValueError:
                    return "invalid"
                return "received"
        if not worker_alive():
            break
        sleep(0.05)
    return "unavailable"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--image-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = inspect(args.directory, args.image_sha256)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
