"""Bind the original SnapshotManager namespace without claiming its pixels."""

import re


def inspect(text, token, specification):
    live, ack = "SWEET LOOP LIVE proto=1 token=" + token, "SWEET LOOP ACK FASTBOOT token=" + token
    if not re.fullmatch("[0-9a-f]{16}", token) or text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
        raise ValueError("Need one original Aegir scope interval")
    scope = text[text.index(live):text.index(ack)]
    identities = list(re.finditer(r"SWEET ASCOPE ID pid=(\d+) value=([0-9a-f]{64})", scope))
    config = list(re.finditer(r"SWEET ASCOPE CONFIG pid=(\d+) revision=1 installed=1 caller=00000000000063b0 old_files_preserved=1 other_calls_original=1", scope))
    if len(identities) != len(config) or not identities:
        raise ValueError("Original namespace installation incomplete")
    pids = {int(match[1]) for match in identities}
    if len(pids) != len(identities) or pids != {int(match[1]) for match in config}:
        raise ValueError("Original namespace process identity differs")
    if any(match[2] != specification["identity"] for match in identities):
        raise ValueError("Original snapshot namespace identity differs")
    call_pattern = re.compile(r"SWEET ASCOPE CALL pid=(\d+) call=(\d+) result=0 original_caller=1 old_files_preserved=1")
    root_pattern = re.compile(r"SWEET ASCOPE ROOT pid=(\d+) call=(\d+) path=(\S+) mode=0700")
    calls, roots = list(call_pattern.finditer(scope)), list(root_pattern.finditer(scope))
    if not calls or len(calls) != len(roots) or len(identities) + len(config) + len(calls) + len(roots) != scope.count("SWEET ASCOPE "):
        raise ValueError("Unknown, truncated or unsuccessful original namespace record")
    keys = set()
    records = []
    for call, root in zip(calls, roots):
        key = int(call[1]), int(call[2])
        if key in keys or key[0] not in pids or not 1 <= key[1] <= 8 or key != (int(root[1]), int(root[2])) or call.end() >= root.start():
            raise ValueError("Original namespace call/root pairing differs")
        keys.add(key)
        allowed = [prefix + "/" + specification["component"] for prefix in (
            "/var/mobile/Library/Caches", "/private/var/mobile/Library/Caches")]
        if root[3] not in allowed:
            raise ValueError("Original namespace root differs")
        records.append(dict(pid=key[0], call=key[1], directory=root[3]))
    return dict(token=token, complete_calls=len(records), records=records,
                namespace_identity=specification["identity"], original_caller_verified=True,
                old_files_preserved=True, original_UI_or_cache_pixels_not_proven=True)
