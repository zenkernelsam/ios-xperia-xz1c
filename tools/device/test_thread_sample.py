"""The existing serial owner sends one read-only sample and refuses a resend."""
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import Mock
from controller import LiveOps
from policy import Refuse


class ThreadSample(unittest.TestCase):
    def test_same_worker_single_command(self):
        with tempfile.TemporaryDirectory(prefix='lilac-thread-sample-fixture-') as directory:
            ops=object.__new__(LiveOps);ops.check_stop=Mock();ops.journal=Mock();ops.directory=Path(directory)
            ops.capture_process=SimpleNamespace(process=SimpleNamespace(poll=lambda:None))
            artifact=SimpleNamespace(token='0123456789abcdef',data={'thread_sample_observation':{'command':'sample'}})
            ops.request_sample(artifact)
            self.assertEqual(json.loads((ops.directory/'serial-commands.jsonl').read_text())['command'],'sample')
            with self.assertRaises(Refuse):ops.request_sample(artifact)
            self.assertEqual(len((ops.directory/'serial-commands.jsonl').read_text().splitlines()),1)
    def test_no_binding_or_dead_worker_refused(self):
        ops=object.__new__(LiveOps);ops.check_stop=Mock();artifact=SimpleNamespace(token='0123456789abcdef',data={})
        with self.assertRaises(Refuse):ops.request_sample(artifact)
        artifact.data={'thread_sample_observation':{'command':'sample'}}
        ops.capture_process=SimpleNamespace(process=SimpleNamespace(poll=lambda:0))
        with self.assertRaises(Refuse):ops.request_sample(artifact)


if __name__=='__main__':unittest.main()
