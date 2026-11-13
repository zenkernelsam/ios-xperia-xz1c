"""Independent bounded Sony health observation; never downloads, boots or resets."""
import argparse
import json
import time
from pathlib import Path
from controller import Journal, LiveOps, HERE, PATCH, write_json
from policy import FastbootStability, Refuse
from procguard import LiveLock


def observe(output, seconds=30):
    assert seconds >= 30
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    journal = Journal(output)
    ops = LiveOps(journal)
    samples = []
    report = dict(serial='BH905ND69G', usb_vid='0fce', usb_pid='0dde',
                  download_commands=0, boot_commands=0, reset_commands=0,
                  independent_health_only=True, previous_trial_result_unchanged=True)
    lock = LiveLock(output / 'device.lock')
    try:
        inventory = ops.command(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                                 '-File', HERE / 'preflight_processes.ps1'], 12, 'process_preflight')
        if json.loads(inventory):
            raise Refuse('Another boot/capture worker is running')
        stability = FastbootStability(seconds)
        start = time.monotonic()
        while True:
            # Native USB identity and serial-targeted endpoint checks each sample.
            ops.command([__import__('sys').executable, HERE / 'sony_usb_guard.py',
                         '--binary', PATCH / 'assets/device-tools/xperable.exe'], 8, 'usb_identity')
            ops.assert_fastboot()
            elapsed = time.monotonic() - start
            state = stability.feed(elapsed, True)
            samples.append(dict(elapsed_seconds=elapsed, healthy=True))
            if state == 'STABLE':
                report.update(result='PASS', continuous_health_seconds=elapsed-stability.since,
                              samples=samples)
                return report
            time.sleep(1)
    except BaseException as error:
        report.update(result='STOP_ERROR', error=str(error), samples=samples)
        raise
    finally:
        ops.close()
        lock.close()
        write_json(output / 'health.json', report)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    print(json.dumps(observe(args.output)))
