"""Deterministic log policy: expected startup activity never means reboot."""
from dataclasses import dataclass
import re

SERIAL = "BH905ND69G"
VID, PID = 0x1209, 0x0001

class Refuse(RuntimeError): pass

class FastbootStability:
    def __init__(self, seconds=30): self.seconds=seconds; self.since=None; self.lost=False
    def feed(self, now, present, healthy=True):
        if self.lost: return "LOST"
        if not present or not healthy:
            if self.since is not None: self.lost=True; return "LOST"
            return "WAIT"
        if self.since is None: self.since=now
        return "STABLE" if now-self.since>=self.seconds else "WAIT"

def unique_fastboot(text):
    lines = [s.split() for s in text.splitlines() if s.strip()]
    if len(lines) != 1 or len(lines[0]) < 2 or lines[0][0] != SERIAL or lines[0][1] != "fastboot":
        raise Refuse("Fastboot inventory must contain exactly BH905ND69G")
    return SERIAL

def serial_candidates(ports):
    result = []
    for p in ports:
        match = re.fullmatch(r"COM(\d+)", p.device, re.I)
        if p.vid == VID and p.pid == PID and match and int(match[1]) >= 10: result.append(p)
    if len(result) != 1: raise Refuse("Need one active VID_1209/PID_0001 CDC on COM>=10")
    return result[0]

@dataclass
class Decision:
    kind: str
    reason: str

class LogPolicy:
    def __init__(self, token, goals, stable_seconds=10):
        self.token = token
        self.goals = [re.compile(x) for x in goals]
        self.seen = set()
        self.live = False
        self.goal_at = None
        self.stable = stable_seconds
        self.rpm_handshake = self.rpm_stuck = self.rpm_clock = False
        self.crashes = []
        self.actual_watchdog = None
        self.bad = None
        self.recoverable = None
    def feed(self, line, now):
        # LIVE response is generated after a token-checked command, unlike a replay marker.
        if re.search(r"SWEET LOOP LIVE proto=1 token=" + re.escape(self.token) + r"(?:\s|$)", line): self.live = True
        if "SWEET LOOP RESET_RETURNED" in line or "SWEET LOOP REJECT" in line:
            self.bad = "RESET_REJECTED_OR_RETURNED"
        m = re.search(r"SWEET: root up, auto-reset deadline -> (\d+) s", line)
        if m: self.actual_watchdog = int(m[1])
        if re.match(r"SWEET RPM: version handshake timeout", line): self.rpm_handshake = True
        if "SWEET RPM:" in line and re.search(r"txh=8\s+txt=0", line): self.rpm_stuck = True
        if re.search(r"SWEET GPU: rpm gpu clocks -> -[1-9]", line): self.rpm_clock = True
        if self.rpm_handshake and self.rpm_stuck and self.rpm_clock: self.bad = "RPM_ALWAYS_ON_STUCK"
        if re.match(r"(?:panic\(cpu\s+\d+|Kernel trap|SWEET PANIC:)", line): self.recoverable = "KERNEL_PANIC"
        if re.search(r"SWEET GPU: (?:timeout dump|submit \d+ timed out|EOP submit.*result=-1)", line):
            self.recoverable = "GPU_SUBMIT_TIMEOUT"
        if re.match(r"CRASH backboardd", line):
            self.crashes = [t for t in self.crashes if now - t <= 60] + [now]
            if len(self.crashes) >= 3: self.recoverable = "BACKBOARDD_CRASH_LOOP"
        for i, expression in enumerate(self.goals):
            if expression.search(line): self.seen.add(i)
        if self.live and len(self.seen) == len(self.goals) and self.goal_at is None: self.goal_at = now
    def decision(self, now):
        if self.bad: return Decision("STOP", self.bad)
        if self.live and self.recoverable: return Decision("REVISE", self.recoverable)
        if self.goal_at is not None and now - self.goal_at >= self.stable: return Decision("PASS", "GOALS_STABLE")
        return Decision("WAIT", "COLLECTING")
