"""Host-only CDC opening, cursor replay and proof fault tests; no ports opened."""
import json
import os
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from controller import Artifact
from ramcpu_prefix_evidence import verify as verify_prefix
from serial_capture_evidence import verify as verify_capture
from serial_worker import capture_binding, capture_stream, main, open_serial

TOKEN = '0123456789abcdef'
LIBRARY = 'SWEET GLRAM: library read=0 bytes=14784032 fnv=40ec75a3250e80ae ro=1'
RAW = (f'SWEET LOOP READY proto=1 token={TOKEN} mode=graceful pon=0x06\n'
       f'SWEET GLRAM: mount result=0\n{LIBRARY}\nDarwin Kernel Version 22.6.0\n'
       f'SWEET LOOP LIVE proto=1 token={TOKEN}\n').encode()


class CDC:
    def __init__(self, **options):
        if options['port'] is not None:
            raise AssertionError('Constructing an already-open port races its purge')
        self.options = options
        self._dtr = True
        self.port = None
        self.is_open = False
        self.closed = False
        self.writes = []
        self.buffer = RAW
        self.root = None
        self.open_error = False
        self.reads = 0

    @property
    def dtr(self): return self._dtr

    @dtr.setter
    def dtr(self, value):
        if self.is_open and value:
            assert self.root and (self.root/'serial.raw.log').exists(), 'No sink before DTR rise'
        self._dtr = value

    def open(self):
        # Model the Windows configure-then-purge ordering: with DTR high the
        # device transmits and advances its cursor before RX is discarded.
        if self.dtr:
            self.buffer = self.buffer[64:]
        self.is_open = True
        if self.open_error: raise OSError('Partial driver open failed')

    def close(self): self.closed = True; self.is_open = False
    def __enter__(self): return self
    def __exit__(self, *args): self.close()
    def flush(self): pass
    @property
    def in_waiting(self): return min(len(self.buffer), 37)

    def write(self, command):
        self.writes.append(command)
        if command == b'replay\n':
            # Pending old IN precedes cursor reset, producing a second READY.
            self.buffer = self.buffer[:37] + RAW
        elif command == ('fastboot ' + TOKEN + '\n').encode():
            self.buffer += (f'SWEET LOOP ACK FASTBOOT token={TOKEN}\n').encode()

    def read(self, length):
        self.reads += 1
        if not self.buffer: raise OSError('Normal disconnect after captured bytes')
        data = self.buffer[:length]; self.buffer = self.buffer[length:]; return data


class WorkerTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parent = os.environ.get('LILAC_HOST_TEST_STAGING')
        cls.staging = Path(tempfile.mkdtemp(prefix='lilac-serial-worker-test-', dir=parent))

    def setUp(self):
        self.root = self.staging/self._testMethodName
        self.root.mkdir()

    def journal(self):
        events = []
        def event(kind, **data):
            events.append(dict(event=kind, **data))
            (self.root/'serial-events.jsonl').write_text('\n'.join(json.dumps(x) for x in events)+'\n')
        return events, event

    def test_dtr_stays_low_through_purge(self):
        port = open_serial(CDC, 'COM17')
        self.assertFalse(port.dtr)
        self.assertEqual(port.port, 'COM17')
        self.assertTrue(port.options['exclusive'])
        self.assertFalse(port.options['dsrdtr'])
        self.assertEqual(port.buffer, RAW)
        port.close()

    def test_failed_partial_open_is_closed(self):
        port = CDC(port=None)
        port.open_error = True
        with self.assertRaises(OSError): open_serial(lambda **options: port, 'COM17')
        self.assertTrue(port.closed)
        self.assertFalse(port.dtr)

    def test_one_stream_preserves_prefix_and_returns_once(self):
        port = open_serial(CDC, 'COM17'); port.root = self.root
        events, event = self.journal()
        binding = capture_binding(TOKEN)
        event('capture_source', binding=binding)
        event('attached', port='COM17', vid=0x1209, pid=1, dtr=False, exclusive=True)
        (self.root/'serial-commands.jsonl').write_text(json.dumps({'command':f'fastboot {TOKEN}'})+'\n')
        self.assertEqual(capture_stream(port, self.root, TOKEN, event), 0)
        raw = (self.root/'serial.raw.log').read_bytes()
        self.assertTrue(port.closed)
        self.assertTrue(raw.startswith(RAW))
        self.assertEqual(port.writes, [('loop '+TOKEN+'\n').encode(), b'status\n', ('fastboot '+TOKEN+'\n').encode()])
        self.assertEqual(verify_prefix(raw, TOKEN, LIBRARY)['raw_library_reads'], 1)
        self.assertTrue(verify_capture(self.root/'serial-events.jsonl', raw, TOKEN, binding)['one_continuous_current_boot_stream'])
        original_events = list(events)
        cases = []
        changed = [dict(x) for x in events]; changed[1]['dtr'] = True; cases.append(changed)
        changed = [dict(x) for x in events]; changed[1]['port'] = 'COM3'; cases.append(changed)
        changed = [dict(x) for x in events]; changed[2], changed[3] = changed[3], changed[2]; cases.append(changed)
        changed = [dict(x) for x in events]; changed[4]['command'] = 'replay'; cases.append(changed)
        changed = [dict(x) for x in events]; changed[0]['binding'] = {**binding, 'worker_sha256':'0'*64}; cases.append(changed)
        for changed in cases:
            (self.root/'serial-events.jsonl').write_text('\n'.join(json.dumps(x) for x in changed)+'\n')
            with self.assertRaises(AssertionError): verify_capture(self.root/'serial-events.jsonl', raw, TOKEN, binding)
        (self.root/'serial-events.jsonl').write_text('\n'.join(json.dumps(x) for x in original_events)+'\n')
        for changed in (raw.replace(b'Darwin Kernel Version', b''), RAW + raw,
                        raw + b'VERMEER DEBUG: replay retained RAM log\n'):
            with self.assertRaises(AssertionError): verify_capture(self.root/'serial-events.jsonl', changed, TOKEN, binding)

    def test_existing_raw_is_not_truncated_and_port_closes(self):
        port = open_serial(CDC, 'COM17'); port.root = self.root
        (self.root/'serial.raw.log').write_bytes(b'old failure bytes')
        events, event = self.journal()
        with self.assertRaises(FileExistsError): capture_stream(port, self.root, TOKEN, event)
        self.assertEqual((self.root/'serial.raw.log').read_bytes(), b'old failure bytes')
        self.assertFalse(port.dtr)
        self.assertTrue(port.closed)
        self.assertEqual(port.writes, [])

    def test_queued_replay_or_second_loop_is_refused(self):
        for command in ('replay', f'loop {TOKEN}'):
            root = self.root/command.split()[0]; root.mkdir()
            port = open_serial(CDC, 'COM17'); port.root = root
            (root/'serial-commands.jsonl').write_text(json.dumps({'command':command})+'\n')
            events = []
            self.assertEqual(capture_stream(port, root, TOKEN, lambda kind, **data: events.append((kind, data))), 5)
            self.assertTrue(port.closed)
            self.assertNotIn((command+'\n').encode(), port.writes[2:])

    def test_changed_source_refused_before_enumeration(self):
        with patch('serial.tools.list_ports.comports') as ports:
            self.assertEqual(main(['--directory', str(self.root), '--token', TOKEN, '--source-sha256', '0'*64]), 6)
            ports.assert_not_called()

    def test_capture_hypothesis_ignores_token_only_changes(self):
        first = dict(source_revision='kernel', cmdline='sweetloop='+TOKEN+' serial=3',
                     serial_capture_binding=capture_binding(TOKEN))
        second_token = '1111111111111111'
        second = {**first, 'cmdline':'sweetloop='+second_token+' serial=3',
                  'serial_capture_binding':capture_binding(second_token)}
        self.assertEqual(Artifact(first).experiment(), Artifact(second).experiment())
        old = {key:value for key,value in first.items() if key!='serial_capture_binding'}
        self.assertNotEqual(Artifact(old).experiment(), Artifact(first).experiment())


if __name__ == '__main__': unittest.main(verbosity=2)
