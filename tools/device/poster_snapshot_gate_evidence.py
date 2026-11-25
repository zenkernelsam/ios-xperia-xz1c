"""Pair original poster requests, view returns and unchanged CGRect callbacks."""

import argparse
import collections
import hashlib
import json
import re
from pathlib import Path

APIS = ("cutout", "max_cutout", "background", "floating", "vista")
ALL_APIS = APIS + ("image_load", "image_set")
METHOD = re.compile(r"SWEET PSG METHOD pid=(\d+) api=(\w+) present=([01]) owner=([01]) installed=([01]) argc=(\d+) exact_ABI=1")
CONFIG = re.compile(r"SWEET PSG CONFIG pid=(\d+) revision=([12]) installed=(\d+) expected=([57]) per_api=8 max_calls=(40|56) max_replies=64 (extra_getters=0|scene_getters=0 path_metadata=1) GPU_work=0")
BEGIN = re.compile(r"SWEET PSG BEGIN pid=(\d+) call=(\d+) api=(\w+) receiver=([0-9a-f]{16}) argument=([0-9a-f]{16}) original_forwarded=1")
REQUEST = re.compile(r"SWEET PSG REQUEST pid=(\d+) call=(\d+) orientation=([0-9a-f]{16}) callback_nil=([01])")
END = re.compile(r"SWEET PSG END pid=(\d+) call=(\d+) api=(\w+) result=([0-9a-f]{16}) outcome=(return|throw) original_behavior=1")
REPLY = re.compile(r"SWEET PSG REPLY pid=(\d+) call=(\d+) reply=(\d+) original_forwarded=1")
RECT0 = re.compile(r"SWEET PSG RECT0 pid=(\d+) call=(\d+) reply=(\d+) x=([0-9a-f]{16}) y=([0-9a-f]{16})")
RECT1 = re.compile(r"SWEET PSG RECT1 pid=(\d+) call=(\d+) reply=(\d+) w=([0-9a-f]{16}) h=([0-9a-f]{16})")
REPLY_END = re.compile(r"SWEET PSG REPLY_END pid=(\d+) call=(\d+) reply=(\d+) outcome=(return|throw) original_behavior=1")
PATH = re.compile(r"SWEET PSG PATH pid=(\d+) call=(\d+) bytes=(\d+) truncated=([01]) getter_error=([01]) pixels_read=0")
PATH_DATA = re.compile(r"SWEET PSG PATH_DATA pid=(\d+) call=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)")


def inspect(text, token, require_current=True):
    if require_current:
        if not re.fullmatch("[0-9a-f]{16}", token):
            raise ValueError("Invalid original token")
        live, ack = "SWEET LOOP LIVE proto=1 token=" + token, "SWEET LOOP ACK FASTBOOT token=" + token
        if text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
            raise ValueError("Need one original current LIVE/ACK interval")
        text = text[text.index(live):text.index(ack)]
    patterns = (METHOD, CONFIG, BEGIN, REQUEST, END, REPLY, RECT0, RECT1, REPLY_END, PATH, PATH_DATA)
    events = sorted((match.start(), kind, match) for kind, pattern in enumerate(patterns) for match in pattern.finditer(text))
    if len(events) != text.count("SWEET PSG "):
        raise ValueError("Unknown or truncated original poster gate record")
    methods, configured, calls, replies, frames = {}, set(), {}, {}, {}
    revisions = {}
    counts = collections.Counter()
    for at, kind, match in events:
        pid = int(match[1])
        if pid <= 0:
            raise ValueError("Invalid original process")
        if kind == 0:
            api = match[2]
            entry = pid, api
            if api not in ALL_APIS or entry in methods or pid in configured:
                raise ValueError("Repeated or out-of-order original hook metadata")
            present, owner, installed = map(int, match.groups()[2:5])
            argc = int(match[6])
            expected_argc = 4 if api in ("cutout", "max_cutout", "vista") else 3 if api in ("image_load", "image_set") else 2
            if installed and (not present or not owner or argc != expected_argc):
                raise ValueError("Installed original hook ABI differs")
            methods[entry] = dict(present=bool(present), owner=bool(owner), installed=bool(installed), arguments=argc)
            continue
        if kind == 1:
            revision, installed, expected, maximum = map(int, match.groups()[1:5])
            wanted = APIS if revision == 1 else ALL_APIS
            if (pid in configured or {api for process, api in methods if process == pid} != set(wanted)
                    or expected != len(wanted) or maximum != len(wanted)*8
                    or match[6] != ("extra_getters=0" if revision == 1 else "scene_getters=0 path_metadata=1")):
                raise ValueError("Incomplete or repeated gate configuration")
            if installed != expected or installed != sum(m["installed"] for (process, _), m in methods.items() if process == pid):
                raise ValueError("Original poster gate installation incomplete")
            configured.add(pid)
            revisions[pid] = revision
            continue
        if pid not in configured:
            raise ValueError("Original call precedes verified installation")
        call = int(match[2])
        key = pid, call
        if not 1 <= call <= (40 if revisions[pid] == 1 else 56):
            raise ValueError("Original call exceeds its bound")
        if kind == 2:
            api = match[3]
            if key in calls or api not in (APIS if revisions[pid] == 1 else ALL_APIS) or int(match[4], 16) == 0:
                raise ValueError("Repeated or invalid original gate call")
            counts[pid, api] += 1
            if counts[pid, api] > 8:
                raise ValueError("Original per-API bound exceeded")
            calls[key] = dict(pid=pid, call=call, api=api, receiver=match[4], argument=match[5],
                              begin_at=at, complete=False)
            continue
        if key not in calls:
            raise ValueError("Original result has no call")
        entry = calls[key]
        if kind == 3:
            if entry["api"] not in ("cutout", "max_cutout") or "request" in entry or entry["complete"]:
                raise ValueError("Invalid or duplicate original request")
            nil = match[4] == "1"
            if nil != (int(entry["argument"], 16) == 0):
                raise ValueError("Original callback identity disagrees with nil flag")
            entry["request"] = dict(orientation_bits=match[3], callback_nil=nil)
        elif kind == 4:
            if entry["complete"] or entry["api"] != match[3]:
                raise ValueError("Repeated or foreign original gate return")
            if entry["api"] in ("cutout", "max_cutout") and ("request" not in entry or int(match[4], 16) != 0):
                raise ValueError("Missing original request or invalid void return")
            if entry["api"] == "image_load" and ("path" not in entry or len(entry.get("_path", b"")) != entry["path"]["bytes"]):
                raise ValueError("Original image path metadata incomplete")
            if entry["api"] == "image_set" and int(match[4], 16) != 0:
                raise ValueError("Invalid original image setter void return")
            entry.update(complete=True, result=match[4], outcome=match[5], end_at=at)
        elif kind == 5:
            reply = int(match[3])
            reply_key = pid, reply
            if (not 1 <= reply <= 64 or reply_key in replies or entry["api"] not in ("cutout", "max_cutout")
                    or "request" not in entry or entry["request"]["callback_nil"]):
                raise ValueError("Repeated, foreign or unexpected original callback")
            replies[reply_key] = dict(pid=pid, call=call, reply=reply, begin_at=at, complete=False)
        elif kind in (6, 7):
            reply = int(match[3])
            rect_key = pid, call, reply
            if reply:
                if (pid, reply) not in replies or replies[pid, reply]["call"] != call or replies[pid, reply]["complete"]:
                    raise ValueError("Geometry has no active original callback")
            elif entry["api"] != "vista" or entry["complete"]:
                raise ValueError("Unexpected original initialization geometry")
            frame = frames.setdefault(rect_key, {})
            fields = ("x", "y") if kind == 6 else ("w", "h")
            if any(name in frame for name in fields) or kind == 7 and not {"x", "y"} <= frame.keys():
                raise ValueError("Repeated or reordered original geometry")
            frame.update(zip(fields, match.groups()[3:]))
        elif kind == 8:
            reply = int(match[3])
            reply_key = pid, reply
            if reply_key not in replies or replies[reply_key]["call"] != call or replies[reply_key]["complete"]:
                raise ValueError("Repeated or unmatched original callback completion")
            frame = frames.get((pid, call, reply), {})
            if set(frame) != {"x", "y", "w", "h"}:
                raise ValueError("Original callback geometry is incomplete")
            replies[reply_key].update(complete=True, outcome=match[4], rectangle_bits=frame, end_at=at)
        elif kind == 9:
            count = int(match[3])
            if entry["api"] != "image_load" or entry["complete"] or "path" in entry or count > 256:
                raise ValueError("Invalid original image path header")
            entry["path"] = dict(bytes=count, truncated=match[4] == "1", getter_error=match[5] == "1")
            entry["_path"] = b""
        else:
            if entry["api"] != "image_load" or entry["complete"] or "path" not in entry:
                raise ValueError("Original image path data has no header")
            offset, count = int(match[3]), int(match[4])
            data = bytes.fromhex(match[5])
            if offset != len(entry["_path"]) or not 1 <= count <= 24 or len(data) != count or offset + count > entry["path"]["bytes"]:
                raise ValueError("Truncated, duplicate or out-of-order original image path")
            entry["_path"] += data
    if not configured or not calls:
        raise ValueError("No original poster gate observations")
    for (pid, call), entry in calls.items():
        if entry["api"] == "vista" and set(frames.get((pid, call, 0), {})) != {"x", "y", "w", "h"}:
            raise ValueError("Original Vista initialization frame is incomplete")
        if entry["api"] == "vista":
            entry["frame_bits"] = frames[pid, call, 0]
        if entry["api"] == "image_load":
            data = entry.pop("_path", b"")
            entry["path"].update(utf8=data.decode("utf-8", errors="replace"), hex=data.hex())
    for pid in configured:
        ids = {call for process, call in calls if process == pid}
        if ids != set(range(1, max(ids, default=0) + 1)):
            raise ValueError("Original bounded call sequence is incomplete")
    pending = [entry for key, entry in calls.items() if entry["api"] in ("cutout", "max_cutout")
               and "request" in entry and not entry["request"]["callback_nil"]
               and not any(reply["pid"] == entry["pid"] and reply["call"] == entry["call"] for reply in replies.values())]
    if len(set(revisions.values())) != 1:
        raise ValueError("Mixed original gate revisions")
    return dict(revision=next(iter(revisions.values())), token=token, processes=sorted(configured), methods=[
        dict(pid=pid, api=api, **info) for (pid, api), info in methods.items()],
        calls=list(calls.values()), replies=list(replies.values()),
        complete_calls=sum(entry["complete"] for entry in calls.values()),
        incomplete_calls=sum(not entry["complete"] for entry in calls.values()),
        complete_replies=sum(entry["complete"] for entry in replies.values()),
        incomplete_replies=sum(not entry["complete"] for entry in replies.values()),
        pending_original_cutout_requests=pending,
        extra_scene_getter_or_GPU_calls=False, bounded_path_metadata_read=next(iter(revisions.values())) == 2,
        original_callbacks_not_synthesized=True,
        missing_reply_is_not_a_global_timeout_proof=True, post_ACK_excluded=require_current)


def verify(directory):
    from scanout_trial_evidence import verify as verify_screen
    root = Path(directory)
    screen = verify_screen(root)
    artifact = json.loads((root / "results.json").read_bytes())[0]["artifact"]
    observer = artifact["exception_observer"]
    if observer.get("poster_snapshot_gate_revision") not in (1, 2) or not observer.get("poster_snapshot_gate_observer"):
        raise ValueError("Original compiled gate binding differs")
    raw = (root / "cycle-01/serial.raw.log").read_bytes()
    if hashlib.sha256(raw).hexdigest() != screen["raw_sha256"]:
        raise ValueError("Original raw identity differs")
    report = inspect(raw.decode("ascii", "replace"), artifact["token"])
    if report["revision"] != observer["poster_snapshot_gate_revision"]:
        raise ValueError("Original gate records and compiled revision differ")
    report.update(raw_sha256=screen["raw_sha256"], original_result=screen["original_result"],
                  original_reason=screen["original_reason"], stage_not_promoted=True,
                  return_seconds_with_health=screen["return_seconds_with_health"], device_operations=0)
    return report


def inspect_image_cache(text, token):
    """Independently bind every image load/set even if another gate record is lost."""
    live, ack = "SWEET LOOP LIVE proto=1 token=" + token, "SWEET LOOP ACK FASTBOOT token=" + token
    if not re.fullmatch("[0-9a-f]{16}", token) or text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
        raise ValueError("Need one original image-cache LIVE/ACK interval")
    scope = text[text.index(live):text.index(ack)]
    configs = list(CONFIG.finditer(scope))
    if len(configs) != 1 or configs[0].groups()[1:5] != ("2", "7", "7", "56"):
        raise ValueError("Image-cache observation needs its actual revision2 installation")
    loads, setters = [], []
    begins = [m for m in BEGIN.finditer(scope) if m[3] in ("image_load", "image_set")]
    seen = set()
    for begin in begins:
        pid, call, api, receiver, argument = begin.groups()
        key = int(pid), int(call)
        if key in seen or not 1 <= key[1] <= 56 or int(receiver, 16) == 0:
            raise ValueError("Invalid or repeated original image-cache call")
        seen.add(key)
        ends = [m for m in END.finditer(scope) if (int(m[1]), int(m[2])) == key]
        if len(ends) != 1 or ends[0][3] != api or begin.end() >= ends[0].start():
            raise ValueError("Original image-cache call is incomplete")
        end = ends[0]
        row = dict(pid=key[0], call=key[1], receiver=receiver, argument=argument,
                   result=end[4], outcome=end[5], begin_at=begin.start(), end_at=end.end())
        if api == "image_set":
            if int(end[4], 16) != 0:
                raise ValueError("Original setter has a nonvoid return")
            setters.append(row)
            continue
        headers = [m for m in PATH.finditer(scope) if (int(m[1]), int(m[2])) == key]
        if len(headers) != 1 or not begin.end() < headers[0].start() < end.start():
            raise ValueError("Original cached image path header is missing")
        header = headers[0]
        count = int(header[3])
        data = bytearray()
        for record in PATH_DATA.finditer(scope):
            if (int(record[1]), int(record[2])) != key:
                continue
            block = bytes.fromhex(record[5])
            if (not header.end() < record.start() < end.start() or int(record[3]) != len(data)
                    or int(record[4]) != len(block) or not 1 <= len(block) <= 24 or len(data) + len(block) > count):
                raise ValueError("Original cached image path bytes are incomplete or reordered")
            data.extend(block)
        if len(data) != count or count > 256:
            raise ValueError("Original cached image path length differs")
        row.update(path_bytes=count, path_hex=data.hex(), path_utf8=data.decode("utf-8"),
                   path_truncated=header[4] == "1", path_getter_error=header[5] == "1")
        loads.append(row)
    if not loads or len(loads) > 8 or len(setters) > 8:
        raise ValueError("No bounded original cached image loads")
    return dict(token=token, loads=loads, setters=setters,
                distinct_paths=sorted({row["path_utf8"] for row in loads}),
                complete_loads=len(loads), complete_setters=len(setters),
                all_load_results_nonnull=all(int(row["result"], 16) > 0 and row["outcome"] == "return" for row in loads),
                unrelated_missing_gate_records_not_repaired=True,
                source_file_pixels_not_read=True, original_failure_not_promoted=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    result = verify(args.directory)
    with Path(args.output).open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(result, stream, indent=2)
        stream.write("\n")
    print(json.dumps({name: result[name] for name in (
        "original_result", "complete_calls", "incomplete_calls", "complete_replies", "incomplete_replies"
    )}))
