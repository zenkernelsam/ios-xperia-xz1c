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
from controller import validate_plan,LiveOps
from policy import Refuse
from run_trial import prerequisites,DiagnosticOps
from scanout_snapshot import binding
from diagnostic_candidate import configuration


class ScreenGates(unittest.TestCase):
    def record(self, version=730):
        config = configuration(version)
        record = dict(version=version, trial_mode='ramcpu', kernel_sha256='a'*64,
            diagnostic_binding=config['binding'], cmdline='sfscreen=1 sweetfb_noconsole=1',
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
        changed=self.record();changed['cmdline']='sfscreen=1 sweetfb_noconsole=0'
        with self.assertRaises(ValueError):binding(changed)

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

    def test_observation_frame_is_reused_without_another_command(self):
        ops=object.__new__(LiveOps);ops.screen_attempted=True;ops.screen_token='0011223344556677'
        ops.screen_result={'verified':True,'frames':['same retained frame']}
        artifact=SimpleNamespace(token=ops.screen_token,data=self.record())
        self.assertIs(ops.screen(None,artifact),ops.screen_result)
        artifact.token='ffeeddccbbaa9988'
        with self.assertRaises(Refuse):ops.screen(None,artifact)

    def test_failed_observation_capture_is_not_resent(self):
        ops=object.__new__(LiveOps);ops.screen_attempted=True;ops.screen_token='0011223344556677';ops.screen_result=None
        artifact=SimpleNamespace(token=ops.screen_token,data=self.record())
        with self.assertRaises(Refuse):ops.screen(None,artifact)

    def test_late_frame_keeps_one_shot_return_and_observation_bounds(self):
        plan=dict(target_serial='BH905ND69G',pon_route=6,max_cycles=1,budget_seconds=600,
            screen_required=True,screen_timeout_seconds=100,steps=[dict(observe_seconds=240,
                return_seconds=120,return_evidence='final-drain',on_pass='stop',on_failure='stop')])
        for seconds in (45,150):
            plan['screen_capture_after_seconds']=seconds
            self.assertEqual(validate_plan(plan),1)
        for invalid in (True,150.0,0,-1,46,149,180,300,'150'):
            plan['screen_capture_after_seconds']=invalid
            with self.subTest(invalid=invalid),self.assertRaises(Refuse):validate_plan(plan)
        plan['screen_capture_after_seconds']=150;plan['steps'][0]['observe_seconds']=150
        with self.assertRaises(Refuse):validate_plan(plan)
        plan['steps'][0]['observe_seconds']=240;plan['screen_required']=False
        with self.assertRaises(Refuse):validate_plan(plan)

    def test_late_observation_dispatches_once_before_normal_return_decision(self):
        now=[0.0];sent=[];delivered=[False]
        ops=object.__new__(DiagnosticOps)
        ops.screen_attempted=False;ops.directory=Path('absent-test-serial-directory')
        ops.journal=SimpleNamespace(root=ops.directory,event=lambda *a,**k:None)
        ops.capture_process=SimpleNamespace(process=SimpleNamespace(poll=lambda:None))
        ops.check_stop=lambda:None
        token='0123456789abcdef'
        def lines():
            if delivered[0]:return []
            delivered[0]=True
            return ['SWEET LOOP LIVE proto=1 token='+token,'native-ready']
        ops.lines=lines
        def capture(*args):
            sent.append(now[0]);ops.screen_attempted=True
            return {'verified':True}
        ops.screen=capture
        def sleep(seconds):now[0]+=1
        artifact=SimpleNamespace(token=token,watchdog=420,
            data={'trial_mode':'gpu','screen_capture_after_seconds':150})
        step={'goals':['native-ready'],'stable_seconds':30,'observe_seconds':240}
        with patch('run_trial.time.monotonic',side_effect=lambda:now[0]), \
             patch('run_trial.time.sleep',side_effect=sleep),patch('run_trial.write_json'):
            decision,live=ops.observe(artifact,step)
        self.assertTrue(live);self.assertEqual(decision.kind,'PASS')
        self.assertEqual(sent,[150.0]);self.assertEqual(now[0],180.0)

    def test_idle_sample_requires_actual_library_and_dispatches_once(self):
        from numeric_boot_args import sampler_binding
        for library_returned in (False,True):
            now=[0.0];sent=[];ready=[False]
            ops=object.__new__(DiagnosticOps);ops.screen_attempted=True
            ops.directory=Path('absent-idle-sample-fixture');ops.journal=SimpleNamespace(root=ops.directory,event=lambda *a,**k:None)
            ops.capture_process=SimpleNamespace(process=SimpleNamespace(poll=lambda:None));ops.check_stop=lambda:None
            token='0123456789abcdef'
            def lines():
                if ready[0]:return []
                ready[0]=True;result=['SWEET LOOP LIVE proto=1 token='+token]
                if library_returned:result+=['|AegirPoster| SWEET METAL LIBRARY END call=1 kind=bundle returned_nil=0 error_pointer=0 error_code=0 original_return=1']
                return result
            ops.lines=lines
            def sample(*a):sent.append(now[0]);ops.sample_requested=True
            ops.request_sample=sample
            artifact=SimpleNamespace(token=token,watchdog=420,data={'trial_mode':'gpu','thread_sample_observation':sampler_binding('after_library_idle')})
            step={'goals':['not-returned-pipeline'],'stable_seconds':30,'observe_seconds':240}
            with patch('run_trial.time.monotonic',side_effect=lambda:now[0]), \
                 patch('run_trial.time.sleep',side_effect=lambda seconds:now.__setitem__(0,now[0]+1)),patch('run_trial.write_json'):
                decision,live=ops.observe(artifact,step)
            self.assertTrue(live);self.assertEqual(decision.kind,'STOP')
            self.assertEqual(sent,[180.0] if library_returned else [])


if __name__=='__main__':unittest.main()
