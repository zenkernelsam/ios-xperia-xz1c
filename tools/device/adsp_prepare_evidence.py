"""Verify the preparation-only ADSP stage from a preserved RAM CPU boot."""

import argparse
import hashlib
import json
import re
from pathlib import Path


BEGIN = b"SWEET ADSP BEGIN stage=3 pas_id=1 power_writes=0 firmware_started=0"
END = (b"SWEET ADSP END stage=3 result=0 verified=1 api_count=3 prepared=1 "
       b"authenticated=0 ready_observed=0 stopped=0 power_requests=0")
FW = re.compile(
    rb"SWEET ADSP FW verified=1 inflate=0 packed=5761449 raw=27262976 "
    rb"fnv=5917c1c2c0cb472b mdt=7260 mdt_fnv=97d7a2020d3de26e "
    rb"target=0x8b200000 loaded=0 authenticated=0"
)
SETUP = re.compile(
    rb"SWEET ADSP AP_SETUP result=0 out=(0x[0-9a-f]{1,8}) desc=(0x[0-9a-f]{1,8}) tx=(0x[0-9a-f]{1,8}) "
    rb"global_free=(0x[0-9a-f]{1,8}) private_free=(0x[0-9a-f]{1,8}) "
    rb"created=([01]/[01]/[01]) lock_released=1 dsp_started=0 unowned_writes=0"
)
FIRMWARE_BLOBS = {
    "ADSP-MDT": {"bytes": 7260, "sha256": "cce471158185ca1b384a9f1cfb5421128fd5c1609f21c3f6caabc065a59158a7"},
    "ADSP-IMAGE-Z": {"bytes": 5761449, "sha256": "b3b620f879153064a33bd3de82b4acfe98f25739d17e5943b040393309d11b44"},
}


def inspect(raw, token):
    if not re.fullmatch(r"[0-9a-f]{16}", token):
        raise ValueError("Invalid current token")
    lines = raw.splitlines()
    encoded = token.encode("ascii")
    live = b"SWEET LOOP LIVE proto=1 token=" + encoded + b" "
    ack = b"SWEET LOOP ACK FASTBOOT token=" + encoded + b" "
    starts = [index for index, line in enumerate(lines) if line.startswith(live)]
    stops = [index for index, line in enumerate(lines) if line.startswith(ack)]
    if len(starts) != 1 or len(stops) != 1 or starts[0] >= stops[0]:
        raise ValueError("Expected one ordered current LIVE/ACK")
    records = [(index + 1, lines[index]) for index in range(starts[0] + 1, stops[0])
               if lines[index].startswith(b"SWEET ADSP ")]
    if len(records) != 7:
        raise ValueError("Expected exactly one complete preparation-only ADSP stage")
    numbers, values = zip(*records)
    if values[0] != BEGIN or values[-1] != END or not FW.fullmatch(values[1]):
        raise ValueError("ADSP preparation or firmware markers differ")
    if values[2:5] != tuple(f"SWEET ADSP API cmd={cmd} result=0 available=1".encode()
                            for cmd in (1, 2, 5)):
        raise ValueError("Unexpected ADSP API availability sequence")
    setup = SETUP.fullmatch(values[5])
    if not setup:
        raise ValueError("AP setup is incomplete or reports an unexpected write")
    offsets = [int(setup[index], 16) for index in (1, 2, 3)]
    if offsets != sorted(set(offsets)) or any(offset == 0 or offset % 8 for offset in offsets):
        raise ValueError("Invalid ADSP AP setup offsets")
    return {
        "stage": 3,
        "line_numbers": list(numbers),
        "firmware_verified": True,
        "api_commands_read_as_available": [1, 2, 5],
        "ap_setup_verified": True,
        "adsp_started": False,
        "apr_transaction_verified": False,
        "audio_output_verified": False,
    }


def verify(directory):
    from trial_evidence import verify as verify_trial

    root = Path(directory).resolve()
    results = (root / "results.json").read_bytes()
    rows = json.loads(results)
    if len(rows) != 1:
        raise ValueError("Expected a single preserved RAM CPU run")
    record = rows[0]["artifact"]
    words = record["cmdline"].split()
    for key, expected in (("sweetadspboot", "3"), ("sweetgpunorpm", "1")):
        if [word for word in words if word == key or word.startswith(key + "=")] != [f"{key}={expected}"]:
            raise ValueError(f"Expected exact preparation-only {key} boot argument")
    if any(record["blobs"].get(key) != value for key, value in FIRMWARE_BLOBS.items()):
        raise ValueError("ADSP firmware identity differs")
    proof = verify_trial(root, "ramcpu")
    raw = (root / "cycle-01/serial.raw.log").read_bytes()
    if hashlib.sha256(raw).hexdigest() != proof["log_sha256"] or (root / "results.json").read_bytes() != results:
        raise ValueError("Preserved evidence changed during verification")
    return dict(token=proof["token"], image_sha256=proof["image_sha256"], raw_sha256=proof["log_sha256"],
                original_result=rows[0]["result"], **inspect(raw, proof["token"]), device_operations=0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.directory)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
