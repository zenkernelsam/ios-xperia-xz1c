"""Read-only Windows PnP observations before the current boot's LIVE token.

This never opens a USB interface, COM port, block device, or recovery command.
It only classifies the exact Sony's metadata and stops the current observation.
"""
import json
import os
import re
import subprocess
from policy import Decision, Refuse

SONY = "BH905ND69G"
IDENTITY = re.compile(r"^USB\\VID_0FCE&PID_([0-9A-F]{4})\\BH905ND69G$", re.I)
PNP_COMMAND = r"""[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false);
Get-PnpDevice -PresentOnly | Where-Object {
    $_.InstanceId -match '^USB\\VID_0FCE&PID_[0-9A-F]{4}\\BH905ND69G$'
} | Select-Object Status,InstanceId | ConvertTo-Json -Compress"""


def parse_pnp(text):
    text = text.lstrip("\ufeff").strip()
    if not text:
        return []
    value = json.loads(text)
    rows = value if isinstance(value, list) else [value]
    if not all(isinstance(row, dict) for row in rows):
        raise ValueError("Invalid PnP observation")
    return rows


def query_pnp(runner=None):
    if os.name != "nt":
        raise OSError("Read-only PnP query requires Windows")
    argv = ["powershell.exe", "-NoProfile", "-NonInteractive", "-Command", PNP_COMMAND]
    if runner is not None:
        return parse_pnp(runner(argv, 8, "startup_pnp"))
    result = subprocess.run(argv,
        capture_output=True, encoding="utf-8", errors="replace", timeout=8,
        creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise OSError(result.stderr.strip() or "PnP query failed")
    return parse_pnp(result.stdout)


class StartupModePolicy:
    """Two consistent observations, with no intervening absence, precede STOP.

    E1F4 needs >=6s; fastboot return without LIVE needs >=30s. Neither
    condition implies boot0, an orderly RPC, or a known kernel failure stage.
    """
    def __init__(self):
        self.candidate = None
        self.since = 0
        self.last = -1
        self.ever_live = False

    def feed(self, now, rows, live=False):
        if now < self.last:
            raise ValueError("Startup time moved backwards")
        self.last = now
        if live:
            self.ever_live = True
        if self.ever_live:
            self.candidate = None
            return None
        modes = set()
        for row in rows:
            match = IDENTITY.fullmatch(str(row.get("InstanceId", "")))
            if match:
                modes.add((match.group(1).upper(), str(row.get("Status", "")).upper()))
        if not modes:
            self.candidate = None
            return None
        mode = tuple(sorted(modes))
        if mode != self.candidate:
            self.candidate, self.since = mode, now
            return None
        seconds = now - self.since
        if len(modes) != 1:
            return Decision("STOP", "STARTUP_USB_MODE_CONFLICT") if seconds >= 6 else None
        pid, status = next(iter(modes))
        if status != "OK":
            return Decision("STOP", "STARTUP_USB_DRIVER_NOT_OK") if seconds >= 6 else None
        if pid == "0DDE":
            return Decision("STOP", "EARLY_FASTBOOT_RETURN_WITHOUT_LIVE") if seconds >= 30 else None
        if pid == "E1F4":
            return Decision("STOP", "STARTUP_USB_E1F4_BEFORE_LIVE") if seconds >= 6 else None
        return Decision("STOP", "STARTUP_USB_UNEXPECTED_" + pid) if seconds >= 6 else None


class StartupObservation:
    """Mixin for LongOps; optional metadata never replaces LIVE verification."""
    def startup_decision(self, artifact, now):
        if getattr(self, "_startup_token", None) != artifact.token:
            self._startup_modes = StartupModePolicy()
            self._startup_next = 0
            self._startup_token = artifact.token
        if now < self._startup_next:
            return None
        self._startup_next = now + 5
        try:
            rows = query_pnp(self.command)
        except (OSError, subprocess.TimeoutExpired, ValueError, Refuse) as error:
            self._startup_modes.feed(now, [])
            self.journal.event("STARTUP_USB_UNAVAILABLE", token=artifact.token, error=str(error))
            return None
        decision = self._startup_modes.feed(now, rows)
        self.journal.event("STARTUP_USB_OBSERVATION", token=artifact.token,
            elapsed_seconds=round(now, 2), devices=rows, metadata_only=True)
        if decision:
            self.journal.event("STARTUP_USB_STOP", token=artifact.token, reason=decision.reason,
                boot_was_dispatched=True, reset_requests=0, recovery_command_sent=False)
        return decision
