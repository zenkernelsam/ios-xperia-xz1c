"""Offline fault replay for migrated transport and strict current one-shot planning."""
import importlib.util,sys,unittest
from pathlib import Path
sys.dont_write_bytecode=True
here=Path(__file__).resolve().parent
from controller import validate_plan
from policy import Refuse
from return_evidence import inspect_return
class PlanTests(unittest.TestCase):
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
