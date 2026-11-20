"""Independently check a source-bound, single current-boot CDC stream."""
import hashlib
import json
from pathlib import Path
import re

from serial_worker import capture_binding


def verify(event_path, raw, token, binding):
    assert isinstance(raw, bytes), 'Serial capture requires original bytes'
    assert binding == capture_binding(token), 'Serial capture source binding changed'
    events_raw = Path(event_path).read_bytes()
    events = [json.loads(line) for line in events_raw.splitlines()]
    assert events and events[0]['event'] == 'capture_source' and events[0]['binding'] == binding
    required = ('capture_source', 'attached', 'log_ready', 'stream_started', 'closed')
    positions = {}
    for kind in required:
        matches = [i for i, event in enumerate(events) if event['event'] == kind]
        assert len(matches) == 1, 'Missing or repeated serial event: ' + kind
        positions[kind] = matches[0]
    assert list(positions.values()) == sorted(positions.values()), 'Serial startup order changed'
    attached = events[positions['attached']]
    match = re.fullmatch(r'COM(\d+)', attached['port'], re.I)
    assert match and int(match[1]) >= 10 and (attached['vid'], attached['pid']) == (0x1209, 1)
    assert attached['dtr'] is False and attached['exclusive'] is True
    ready = events[positions['log_ready']]
    assert (ready['file'], ready['offset'], ready['mode']) == ('serial.raw.log', 0, 'xb')
    stream = events[positions['stream_started']]
    assert stream['dtr'] is True and stream['replay_commands'] == 0
    tx = [(i, event['command']) for i, event in enumerate(events) if event['event'] == 'tx']
    assert [command for _, command in tx[:2]] == [f'loop {token}', 'status']
    assert tx[0][0] == positions['stream_started'] + 1 and tx[1][0] == tx[0][0] + 1
    assert all(positions['stream_started'] < i < positions['closed'] for i, _ in tx)
    assert [command for _, command in tx].count(f'loop {token}') == 1
    assert not any(command == 'replay' for _, command in tx)
    assert [command for _, command in tx].count(f'fastboot {token}') == 1
    assert not any(event['event'] in ('refuse', 'limit') for event in events)
    prefix = raw.split(('SWEET LOOP LIVE proto=1 token=' + token).encode(), 1)
    assert len(prefix) == 2, 'Current-boot LIVE missing'
    assert raw.count(('SWEET LOOP LIVE proto=1 token=' + token).encode()) == 1
    assert re.findall(rb'SWEET LOOP READY proto=1 token=([0-9a-f]{16})', raw) == [token.encode()]
    assert prefix[0].count(b'Darwin Kernel Version') == raw.count(b'Darwin Kernel Version') == 1, \
        'Current-boot banner missing or repeated'
    assert b'VERMEER DEBUG: replay retained RAM log' not in raw
    return dict(revision=binding['revision'], worker_sha256=binding['worker_sha256'],
                event_log_sha256=hashlib.sha256(events_raw).hexdigest(),
                raw_sha256=hashlib.sha256(raw).hexdigest(), serial_port=attached['port'],
                dtr_low_through_open=True, raw_log_before_stream=True,
                one_continuous_current_boot_stream=True, replay_commands=0,
                old_failure_not_reclassified=True)
