"""Keep healthy Fastboot and complete current-boot shutdown evidence distinct."""
import hashlib
import re

def inspect_return(data, token, checkpoint_bytes, *, require_drain=False):
    if not isinstance(data, bytes) or not re.fullmatch(r"[0-9a-f]{16}", token):
        raise ValueError("Invalid return log/token")
    if not isinstance(checkpoint_bytes, int) or not 0 <= checkpoint_bytes <= len(data):
        raise ValueError("Invalid pre-reset log checkpoint")
    current = token.encode()
    prefix = data[:checkpoint_bytes].splitlines()
    tail = data[checkpoint_bytes:].splitlines()
    live = any(line.startswith(b"SWEET LOOP LIVE proto=1 token=" + current + b" ") for line in prefix)
    expected = (
        b"SWEET LOOP ACK FASTBOOT token=" + current + b" mode=graceful pon=0x06",
        b"SWEET LOOP DRAIN token=" + current + b" mode=graceful pon=0x06 backstop=120",
        b"SWEET LOOP RESET pon=0x06 after graceful shutdown",
    )
    if require_drain:
        expected += (b"SWEET LOOP FINAL_DRAIN token=" + current + b" max_ms=250",)
    positions = [next((i for i, line in enumerate(tail) if line == marker), None) for marker in expected]
    rejected = any(line.startswith((b"SWEET LOOP RESET_RETURNED", b"SWEET LOOP REJECT", b"SWEET LOOP DUPLICATE"))
                   for line in tail)
    ordered = all(p is not None for p in positions) and all(a < b for a, b in zip(positions, positions[1:]))
    complete = live and ordered and not rejected
    reason = "COMPLETE_CURRENT_BOOT_RETURN" if complete else (
        "RETURN_REJECTED_OR_DUPLICATE" if rejected else
        "PRE_RESET_LIVE_MISSING" if not live else
        "FINAL_RESET_OR_DRAIN_MISSING" if any(p is None for p in positions) else
        "RETURN_MARKERS_OUT_OF_ORDER")
    return {"complete": complete, "reason": reason, "current_token": token,
            "checkpoint_bytes": checkpoint_bytes, "log_bytes": len(data),
            "log_sha256": hashlib.sha256(data).hexdigest(), "require_final_drain": require_drain,
            "marker_positions": positions, "final_reset_marker_seen": positions[2] is not None}
