"""Summarize peripheral observations from a verified, already captured run."""

import argparse
import hashlib
import json
import re
from pathlib import Path

from scanout_trial_evidence import verify as verify_scanout


DAEMONS = ("mediaserverd", "bluetoothd", "wifid")
MATCH = re.compile(rb"IOK (backboardd|SpringBoard): matching1? (.+) -> (none|0 found)")
BACKLIGHT_MATCH = re.compile(
    rb"<key>IOPropertyMatch</key><dict(?: ID=\"\d+\")?>"
    rb"<key>backlight-control</key><true/></dict>"
)
WLAN_MATCH = re.compile(rb'<key>IONameMatch</key><string(?: ID="\d+")?>wlan</string>')
BACKLIGHT_PATH = re.compile(rb"IOK (backboardd|SpringBoard): path IODeviceTree:/backlight -> none")


def summarize(raw, token):
    if not re.fullmatch(r"[0-9a-f]{16}", token):
        raise ValueError("Invalid current boot token")
    encoded_token = token.encode("ascii")
    lines = raw.splitlines()
    start = b"SWEET LOOP LIVE proto=1 token=" + encoded_token + b" "
    end = b"SWEET LOOP ACK FASTBOOT token=" + encoded_token + b" "
    starts = [index for index, line in enumerate(lines) if line.startswith(start)]
    ends = [index for index, line in enumerate(lines) if line.startswith(end)]
    if len(starts) != 1 or len(ends) != 1 or starts[0] >= ends[0]:
        raise ValueError("Need one ordered current-token LIVE/ACK pair")

    display_requests, backlight_misses, wlan_misses = [], [], []
    executions = {daemon: [] for daemon in DAEMONS}
    for index in range(starts[0] + 1, ends[0]):
        line = lines[index]
        line_number = index + 1
        if line in (b"SWEET FB: display on", b"SWEET FB: display off"):
            display_requests.append({"line": line_number, "requested": line.rsplit(b" ", 1)[1].decode("ascii")})
        path = BACKLIGHT_PATH.fullmatch(line)
        if path:
            backlight_misses.append({"line": line_number, "caller": path[1].decode("ascii"),
                                     "query": "device_tree_path"})
        match = MATCH.fullmatch(line)
        if match:
            observation = {"line": line_number, "caller": match[1].decode("ascii"),
                           "query": "service_match", "result": match[3].decode("ascii")}
            if BACKLIGHT_MATCH.search(match[2]):
                backlight_misses.append(observation)
            if WLAN_MATCH.search(match[2]):
                wlan_misses.append(observation)
        if line.startswith(b"exec "):
            daemon = line[5:].decode("ascii", "replace")
            if daemon in executions:
                executions[daemon].append(line_number)

    return {
        "display_power_requests": display_requests,
        "backlight_lookup_misses": backlight_misses,
        "wlan_lookup_misses": wlan_misses,
        "daemon_exec_lines": executions,
    }


def verify(directory):
    root = Path(directory).resolve()
    scanout = verify_scanout(root)
    raw = (root / "cycle-01/serial.raw.log").read_bytes()
    if hashlib.sha256(raw).hexdigest() != scanout["raw_sha256"]:
        raise ValueError("Raw log changed after verification")
    return {
        "source": str(root),
        "token": scanout["token"],
        "raw_sha256": scanout["raw_sha256"],
        "original_result": scanout["original_result"],
        "scope": "current LIVE to ACK, exclusively",
        "observations": summarize(raw, scanout["token"]),
        "hardware_function_verified": False,
        "device_operations": 0,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.directory)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps({key: report[key] for key in ("token", "raw_sha256", "original_result")}))
