"""Require the preserved, actual v676 CPU trial and its three distinct RPM ACKs.

This is historical clock evidence, not a promise that the next boot succeeds.
v676 predates FINAL_DRAIN; new trials still require the four current markers.
"""
import hashlib
import json
import re
from pathlib import Path

KERNEL = '2d29dcf1c0fbd3b93d8e67ca445f7916846d05dd35cf5ff0a41727567c66fe8f'
IMAGE = '796057957b47965e9cd30912008fcda957454e3700ff4c196d672f1b9ded07ab'


def verify(directory):
    root = Path(directory).resolve()
    rows = json.loads((root / 'results.json').read_text(encoding='utf-8'))
    assert len(rows) == 1
    row = rows[0]
    assert row['simulated'] is False and row['result'] == 'PASS' and row['live']
    assert row['boot_dispatches'] == row['reset_requests'] == 1
    assert row['fastboot_returned'] and row['reset_ack']
    assert row['reset_to_fastboot_seconds'] >= 30
    record = row['artifact']
    assert record['version'] == 676 and record['kernel_sha256'] == KERNEL
    assert record['sha256'] == IMAGE
    data = (root / 'cycle-01/serial.raw.log').read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    assert digest == row['log_sha256']
    checkpoint = row['checkpoint_log_bytes']
    assert 0 < checkpoint < len(data)
    assert hashlib.sha256(data[:checkpoint]).hexdigest() == row['checkpoint_log_sha256']
    text = data.decode('ascii', 'replace')
    token = record['token']
    assert re.fullmatch('[0-9a-f]{16}', token)
    live = text.index('SWEET LOOP LIVE proto=1 token=' + token)
    ack = text.index('SWEET LOOP ACK FASTBOOT token=' + token)
    drain = text.index('SWEET LOOP DRAIN token=' + token)
    reset = text.index('SWEET LOOP RESET pon=0x06 after graceful shutdown')
    assert live < ack < drain < reset
    assert data.index(('SWEET LOOP ACK FASTBOOT token=' + token).encode()) >= checkpoint
    scope = text[live:ack]
    assert 'SWEET RPM PROBE RESULT result=0 mode=2 clocks=1 gpu=0' in scope
    args = dict(w.split('=', 1) for w in record['cmdline'].split() if '=' in w)
    assert args['sweetloop'] == token and args['sweetgpunorpm'] == '1' and args['sweetrpmprobe'] == '2'
    patterns = [r'clk 0x306b6c63/0 key=0x62616e45 val=1 set=0 -> reply (0x[0-9a-f]+) err=0',
                r'clk 0x6563/0 key=0x7a484b val=85710 set=0 -> reply (0x[0-9a-f]+) err=0',
                r'clk 0x72676761/2 key=0x7a484b val=300000 set=0 -> reply (0x[0-9a-f]+) err=0']
    replies = []
    for pattern in patterns:
        matches = re.findall(pattern, scope)
        assert len(matches) == 1, 'Missing or duplicate actual clock ACK'
        replies.append(matches[0])
    assert len(set(replies)) == 3
    return {'source_run': str(root), 'token': token, 'image_sha256': IMAGE,
            'kernel_sha256': KERNEL, 'log_sha256': digest, 'clock_reply_ids': replies,
            'actual_three_clock_acks': True, 'cpu_returned_to_fastboot': True,
            'historical_clock_evidence_only': True}


if __name__ == '__main__':
    import sys
    print(json.dumps(verify(sys.argv[1]), indent=2))
