"""Read-only tool retirement gate before the single Sony guard mutation."""

import time

from policy import Refuse
from procguard import active_tool_processes


def wait_idle(query_legacy, *, check_stop=lambda: None, emit=lambda **_: None,
              stable_seconds=30, timeout_seconds=120, clock=time.monotonic, pause=time.sleep,
              query_fast=active_tool_processes):
    if not 30 <= stable_seconds <= 120 or not stable_seconds <= timeout_seconds <= 600:
        raise Refuse("Invalid bounded pre-guard idle gate")
    start = clock()
    since = None
    previous = None
    while clock() - start < timeout_seconds:
        check_stop()
        now = clock()
        fast = query_fast()
        current = tuple(sorted((row["pid"], row["exe"]) for row in fast))
        if current != previous:
            emit(event="HOST_IDLE_STATE", conflicts=fast, elapsed_seconds=now-start)
            previous = current
        if fast:
            since = None
        elif since is None:
            since = now
        elif now - since >= stable_seconds:
            # Include the original slow legacy-flow preflight, then recheck
            # the current fast snapshot. No guard/download/boot is repeated.
            legacy = query_legacy()
            if legacy or query_fast():
                since = None
                emit(event="HOST_IDLE_RACE", elapsed_seconds=clock()-start)
            else:
                emit(event="HOST_IDLE_READY", elapsed_seconds=clock()-start,
                     continuous_idle_seconds=clock()-since)
                return
        pause(1)
    raise Refuse("External tool did not retire before the bounded single guard")
