"""Offline fault replay for migrated transport and strict current one-shot planning."""
import importlib.util,sys,unittest
from pathlib import Path
sys.dont_write_bytecode=True
here=Path(__file__).resolve().parent
from controller import validate_plan
from policy import Refuse, unique_fastboot
from sony_usb_guard import target_fastboot_lines, validate_sony_records
from controller import LiveOps
from unittest.mock import Mock, patch
from return_evidence import inspect_return
class PlanTests(unittest.TestCase):
    def test_sony_target_with_unrelated_devices(self):
        for text in ('BH905ND69G\tfastboot\n', 'HA2A1PT2\tfastboot\nBH905ND69G\tfastboot\n',
                     'BH905ND69G fastboot\nHA2A1PT2 fastboot\n'):
            self.assertEqual(unique_fastboot(text),'BH905ND69G')
        for text in ('', 'HA2A1PT2 fastboot\n', '???????????? fastboot\n'):
            self.assertFalse(target_fastboot_lines(text))
            with self.assertRaises(Refuse):unique_fastboot(text)
        for text in ('BH905ND69G fastboot\nBH905ND69G fastboot', 'BH905ND69G',
                     'BH905ND69G offline', 'BH905ND69G fastboot unexpected'):
            with self.assertRaises(Refuse):unique_fastboot(text)
    def test_usb_target_identity_still_required(self):
        row=dict(vid=0x0fce,pid=0x0dde,serial='BH905ND69G')
        self.assertEqual(validate_sony_records([row]),'BH905ND69G')
        for records in ([],[row,row],[dict(row,serial='HA2A1PT2')],[dict(row,vid=0x1209)],
                        [dict(row,pid=1)],[dict(row,serial=None)]):
            with self.assertRaises(RuntimeError):validate_sony_records(records)
    def test_return_tracks_sony_independently_of_other_phone(self):
        for scenario in ('return', 'lost', 'invalid'):
            now=[0.0]
            ops=LiveOps(Mock());ops.lines=lambda:[];ops.check_stop=lambda:None
            def command(argv, *args):
                if str(argv[1])=='devices':
                    if now[0]<3 or (scenario=='lost' and now[0]>=10):return 'HA2A1PT2 fastboot\n'
                    if scenario=='invalid':return 'BH905ND69G offline\nHA2A1PT2 fastboot\n'
                    return 'BH905ND69G fastboot\nHA2A1PT2 fastboot\n'
                self.assertEqual([str(x) for x in argv[1:]],['-s','BH905ND69G','getvar','version-bootloader'])
                return 'version-bootloader: test\n'
            ops.command=command
            def sleep(seconds):now[0]+=seconds
            with patch('controller.time.monotonic',side_effect=lambda:now[0]),patch('controller.time.sleep',side_effect=sleep):
                if scenario=='invalid':
                    with self.assertRaises(Refuse):ops.wait_return(Mock(token='0'*16),45)
                else:
                    returned,_,elapsed=ops.wait_return(Mock(token='0'*16),45)
                    self.assertEqual(returned,scenario=='return')
                    if returned:self.assertGreaterEqual(elapsed,33)
    def plan(self):
        return {'target_serial':'BH905ND69G','pon_route':6,'max_cycles':1,'budget_seconds':480,
                'steps':[{'observe_seconds':180,'return_seconds':120,'return_evidence':'final-drain','goals':['READY'],'on_pass':'stop','on_failure':'stop'}]}
    def test_strict_one_shot(self):
        plan=self.plan();self.assertEqual(validate_plan(plan),1)
        for count in [0,2,5]:
            plan=self.plan();plan['max_cycles']=count
            with self.assertRaises(Refuse):validate_plan(plan)
    def test_no_repeat_or_legacy_return(self):
        for key,value in [('on_failure','repeat'),('on_pass','next'),('return_evidence','legacy')]:
            plan=self.plan();plan['steps'][0][key]=value
            with self.assertRaises(Refuse):validate_plan(plan)
    def test_return_requires_current_token_all_markers(self):
        token='0123456789abcdef'
        prefix=('SWEET LOOP LIVE proto=1 token='+token+' mode=graceful pon=0x06\n').encode()
        tail=[f'SWEET LOOP ACK FASTBOOT token={token} mode=graceful pon=0x06',
              f'SWEET LOOP DRAIN token={token} mode=graceful pon=0x06 backstop=120',
              'SWEET LOOP RESET pon=0x06 after graceful shutdown',
              f'SWEET LOOP FINAL_DRAIN token={token} max_ms=250']
        data=prefix+('\n'.join(tail)+'\n').encode()
        self.assertTrue(inspect_return(data,token,len(prefix),require_drain=True)['complete'])
        for i in range(4):
            wrong=prefix+('\n'.join(tail[:i]+tail[i+1:])+'\n').encode()
            self.assertFalse(inspect_return(wrong,token,len(prefix),require_drain=True)['complete'])
        self.assertFalse(inspect_return(data,'1111111111111111',len(prefix),require_drain=True)['complete'])
if __name__=='__main__':unittest.main(verbosity=2)
