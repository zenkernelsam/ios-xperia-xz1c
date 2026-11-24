"""Compare observed Aegir cache events with a verified same-build inventory."""

import argparse
import hashlib
import json
import re
from pathlib import Path


PREFIX = b"|AegirPoster| SWEET PROGRAM CACHE "
REQUEST = re.compile(rb"request result=0 vertex_bytes=(\d{1,8}) fragment_bytes=(\d{1,8}) immutable=1")
HIT = re.compile(rb"hit bytes=(\d{1,8}) format=(0x[0-9a-f]{1,8}) exact_build=1 exact_source_config=1")
TIMEOUT = b"CRASH AegirPoster reason ns 7 code 0x3: extension check-in timeout"
RECORD_HEADER_BYTES = 136


def summarize(raw, token, scope, inventory):
    if not re.fullmatch(r"[0-9a-f]{16}", token):
        raise ValueError("Invalid observation token")
    lines = raw.splitlines()
    encoded_token = token.encode("ascii")
    live = b"SWEET LOOP LIVE proto=1 token=" + encoded_token + b" "
    ack = b"SWEET LOOP ACK FASTBOOT token=" + encoded_token + b" "
    starts = [index for index, line in enumerate(lines) if line.startswith(live)]
    stops = [index for index, line in enumerate(lines) if line.startswith(ack)]
    if len(starts) != 1 or len(stops) != 1 or starts[0] >= stops[0]:
        raise ValueError("Expected one ordered LIVE/ACK for the observation")
    identity = b"SCOPE identity=" + scope["identity"].encode("ascii")
    root_pattern = re.compile(
        rb"SCOPE result=0 inherited=[01] directory=" + re.escape(scope["directory"].encode("ascii")) +
        rb" mode=1777 old_files_preserved=1"
    )
    requests, hits, timeouts, stores, rejects = [], [], [], [], []
    complete_scopes, pending_identity, active_scope = 0, False, False
    request_files = [row for row in inventory["records"] if row["kind"] == "R"]
    binary_files = {row["key"]: row for row in inventory["records"] if row["kind"] == "B"}
    for number in range(starts[0] + 1, stops[0]):
        line = lines[number]
        line_number = number + 1
        if line == TIMEOUT:
            timeouts.append(line_number)
            active_scope = False
        if not line.startswith(PREFIX):
            continue
        event = line[len(PREFIX):]
        if event.startswith(b"SCOPE identity="):
            if pending_identity or event != identity:
                raise ValueError("Unexpected Aegir cache scope identity")
            pending_identity, active_scope = True, False
            continue
        if event.startswith(b"SCOPE "):
            if not pending_identity or not root_pattern.fullmatch(event):
                raise ValueError("Aegir cache scope is missing, incomplete or failed")
            pending_identity, active_scope = False, True
            complete_scopes += 1
            continue
        if not active_scope:
            raise ValueError("Aegir cache event outside a confirmed scope")
        if event.startswith(b"request result=0 "):
            match = REQUEST.fullmatch(event)
            if match is None:
                raise ValueError("Incomplete successful cache request")
            lengths = tuple(map(int, match.groups()))
            keys = sorted(row["key"] for row in request_files
                          if (row["vertex_bytes"], row["fragment_bytes"]) == lengths)
            requests.append(dict(line=line_number, source_lengths=lengths, request_keys_by_length=keys,
                                 paired_binary_keys=[key for key in keys if key in binary_files]))
        elif event.startswith(b"hit "):
            match = HIT.fullmatch(event)
            if match is None:
                raise ValueError("Incomplete Aegir cache hit")
            count, fmt = int(match[1]), int(match[2], 16)
            keys = sorted(row["key"] for row in binary_files.values()
                          if row["bytes"] == count + RECORD_HEADER_BYTES and row["format"] == fmt)
            hits.append(dict(line=line_number, binary_payload_bytes=count, format=fmt,
                             binary_keys_by_size_and_format=keys))
        elif event.startswith(b"stored "):
            stores.append(line_number)
        elif event.startswith(b"reject "):
            rejects.append(line_number)
    if pending_identity or not complete_scopes:
        raise ValueError("No complete Aegir cache scope")
    return dict(confirmed_scope_pairs=complete_scopes, recorded_requests=requests, recorded_import_hits=hits,
                store_lines=stores, reject_lines=rejects, checkin_timeout_lines=timeouts,
                post_ack_checkin_timeout_count=lines[stops[0]+1:].count(TIMEOUT),
                inventory_candidates_are_metadata_matches_only=True, runtime_keys_not_observed=True,
                cross_boot_inventory_does_not_prove_runtime_import=True, original_wallpaper_verified=False)


def verify_failed_capture(root):
    """Admit original serial events from a returned run with a rejected frame."""
    from controller import Artifact, validate_plan
    from package_trial import ROOT, prove_payload
    from return_evidence import inspect_return
    from scanout_snapshot import ScreenDecoder
    from serial_capture_evidence import verify as verify_capture

    def load(path):
        return json.loads(path.read_bytes())

    def lines(path):
        return [json.loads(line) for line in path.read_bytes().splitlines()]

    row, = load(root / "results.json")
    artifact = row["artifact"]
    token = artifact["token"]
    assert (row["result"], row["decision"], row["reason"]) == (
        "STOP", "STOP", "SCREEN_EVIDENCE_MISSING")
    assert row["simulated"] is False and row["live"] is True
    assert row["boot_dispatches"] == row["reset_requests"] == 1
    assert row["fastboot_returned"] and row["reset_ack"] and row["reset_to_fastboot_seconds"] >= 30
    assert artifact["trial_mode"] == "gpu" and artifact["version"] >= 730
    assert artifact.get("screen_snapshot_binding") and not row.get("screen")
    Artifact(artifact).validate()
    assert prove_payload(artifact) == load(root / "payload-audit.json")
    inputs = load(root / "stage-inputs.json")
    bundle = (ROOT / inputs["bundle"]).resolve()
    assert bundle.is_relative_to(ROOT / "build") and inputs["live"] is True
    assert load(bundle / "artifacts.json")["artifacts"] == [artifact]
    plan = load(bundle / "plan.json")
    assert validate_plan(plan) == 1 and plan["screen_required"] is True and plan["camera_required"] is False
    assert plan["screen_capture_after_seconds"] == artifact["screen_capture_after_seconds"]

    cycle = root / "cycle-01"
    wire = load(cycle / "boot-wire.json")
    assert wire == row["boot_wire"] and wire["serial"] == "BH905ND69G" and wire["retries"] == 0
    assert wire["download_command_attempts"] == wire["boot_command_attempts"] == 1
    assert wire["image_sha256"] == artifact["sha256"] and wire["payload_acknowledged"] is True
    raw = (cycle / "serial.raw.log").read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    checkpoint = row["checkpoint_log_bytes"]
    assert digest == row["log_sha256"] and hashlib.sha256(raw[:checkpoint]).hexdigest() == row["checkpoint_log_sha256"]
    returned = inspect_return(raw, token, checkpoint, require_drain=True)
    assert returned["complete"] and returned == row["return_log_evidence"]
    assert returned == load(cycle / "return-evidence.json")
    capture = cycle / "serial-events.jsonl"
    verify_capture(capture, raw, token, artifact["serial_capture_binding"])
    sent = [event["command"] for event in lines(capture) if event["event"] == "tx"]
    assert sent == ["loop " + token, "status", "frame", "fastboot " + token]
    events = lines(root / "events.jsonl")
    states = [event["state"] for event in events]
    assert "SCREEN_VERIFIED" not in states
    ordered = ["SCREEN_REQUESTED", "SCREEN_UNAVAILABLE", "RESET_REQUESTED", "FASTBOOT_HEALTHY"]
    assert all(states.count(state) == 1 for state in ordered)
    assert [states.index(state) for state in ordered] == sorted(states.index(state) for state in ordered)
    request = events[states.index("SCREEN_REQUESTED")]
    assert request["token"] == token and request["command_dispatches"] == 1
    assert 0 <= request["offset"] < checkpoint
    assert events[states.index("SCREEN_UNAVAILABLE")]["error"] == row["screen_error"]
    try:
        ScreenDecoder(token).feed(raw[request["offset"]:checkpoint])
    except ValueError as error:
        if str(error) != row["screen_error"]:
            raise ValueError("Original frame rejection did not reproduce") from error
    else:
        raise ValueError("Original frame rejection did not reproduce")
    return dict(token=token, raw_sha256=digest, kernel_sha256=artifact["kernel_sha256"],
                version=artifact["version"], original_result=row["result"],
                original_reason=row["reason"], independent_scanout_verified=False)


def verify(observation_directory, inventory_directory, *, failed_capture=False):
    from program_cache_scope_evidence import verify as verify_scope
    from scanout_trial_evidence import verify as verify_scanout
    from trial_evidence import verify as verify_trial

    observation_root = Path(observation_directory)
    inventory_root = Path(inventory_directory)
    observation = (verify_failed_capture(observation_root) if failed_capture
                   else verify_scanout(observation_root))
    reference = verify_trial(inventory_root, "gpu", observation["kernel_sha256"], observation["version"])
    observed = json.loads((observation_root / "results.json").read_bytes())[0]["artifact"]
    background = json.loads((inventory_root / "results.json").read_bytes())[0]["artifact"]
    scope = observed["exception_observer"]["program_cache_scope"]
    if (observed["exception_observer"]["target_process"] != "AegirPoster" or
            background["exception_observer"]["target_process"] != "backboardd" or
            scope != background["exception_observer"]["program_cache_scope"] or
            observed["metal_resources"] != background["metal_resources"] or
            observed["ram_library_binding"]["library_sha256"] != background["ram_library_binding"]["library_sha256"]):
        raise ValueError("Aegir observation and verified inventory must use the same cache scope and libraries")
    inventory = reference["exceptions"]["Program_cache_inventory"]
    last = inventory["snapshots"].get("A", inventory["snapshots"]["B"])
    raw = (observation_root / "cycle-01/serial.raw.log").read_bytes()
    if hashlib.sha256(raw).hexdigest() != observation["raw_sha256"]:
        raise ValueError("Aegir raw changed after verification")
    evidence = summarize(raw, observation["token"], scope, last)
    verified_scope = verify_scope(raw.decode("ascii", "replace"), observation["token"], scope, "AegirPoster")
    if evidence["confirmed_scope_pairs"] != verified_scope["complete_samples"]:
        raise ValueError("Aegir cache scope counts disagree")
    return dict(observation_token=observation["token"], observation_raw_sha256=observation["raw_sha256"],
                original_result=observation["original_result"], original_reason=observation["original_reason"],
                inventory_token=reference["token"], inventory_raw_sha256=reference["log_sha256"],
                scope_identity=scope["identity"], inventory_counts=last["end"],
                independent_scanout_verified=not failed_capture,
                **evidence, device_operations=0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--observation", required=True)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--failed-capture", action="store_true",
                        help="reproduce a captured frame's original rejection; do not admit scanout")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = verify(args.observation, args.inventory, failed_capture=args.failed_capture)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps({name: report[name] for name in ("observation_token", "inventory_token", "original_result")}))
