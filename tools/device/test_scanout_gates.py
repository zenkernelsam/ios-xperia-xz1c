"""The new UI evidence route requires actual screenshots and retains return rules."""
import json
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from controller import validate_plan
from policy import Refuse
from run_trial import prerequisites
from scanout_snapshot import binding
from diagnostic_candidate import configuration


class ScreenGates(unittest.TestCase):
    def record(self, version=730):
        config = configuration(version)
        record = dict(version=version, trial_mode='ramcpu', kernel_sha256='a'*64,
            diagnostic_binding=config['binding'], cmdline='sfscreen=1',
            ram_library_binding={'library_sha256':'b'*64})
        if version >= 730: record['screen_snapshot_binding'] = binding(record)
        return record

    def test_same_kernel_private_must_have_successful_capture(self):
        args = SimpleNamespace(private_evidence='actual-private')
        with patch('run_trial.verify', return_value={'mode':'private'}):
            with self.assertRaises(AssertionError): prerequisites(self.record(), args)
        with patch('run_trial.verify', return_value={'mode':'private', 'screen':{'current_boot_source_and_payload_checked':True}}) as verify:
            result = prerequisites(self.record(), args)
            self.assertTrue(result['private']['screen']['current_boot_source_and_payload_checked'])
            verify.assert_called_once_with('actual-private','private','a'*64,730)

    def test_old_camera_failure_does_not_gain_a_screenshot_binding(self):
        old = self.record(729)
        with self.assertRaises(ValueError): binding(old)
        changed = self.record()
        changed['diagnostic_binding']['manifest_sha256'] = '0'*64
        with self.assertRaises(AssertionError): binding(changed)

    def test_frame_source_changes_need_an_independent_contract(self):
        current = binding(self.record())
        self.assertEqual((current['width'],current['height'],current['stride'],current['format']),(720,1280,2160,'BGR888'))
        self.assertEqual(len(current['reader_source_sha256']),64)
        self.assertEqual(len(current['codec_source_sha256']),64)

    def test_bounded_evidence_and_complete_return_are_required(self):
        plan=dict(target_serial='BH905ND69G',pon_route=6,max_cycles=1,budget_seconds=600,screen_required=True,
            screen_timeout_seconds=100,steps=[dict(observe_seconds=240,return_seconds=120,
                return_evidence='final-drain',on_pass='stop',on_failure='stop')])
        self.assertEqual(validate_plan(plan),1)
        for key, value in (('screen_timeout_seconds',111),('screen_required',1),('pon_route',1),('max_cycles',2)):
            changed=json.loads(json.dumps(plan));changed[key]=value
            with self.assertRaises(Refuse):validate_plan(changed)
        changed=json.loads(json.dumps(plan));changed['steps'][0]['return_evidence']='legacy'
        with self.assertRaises(Refuse):validate_plan(changed)


if __name__=='__main__':unittest.main()
