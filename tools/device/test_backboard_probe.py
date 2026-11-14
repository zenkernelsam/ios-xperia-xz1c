import unittest
from backboard_probe_evidence import verify
from package_trial import observer_environment
class BackgroundProbe(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET ENV: backboardd += DYLD_INSERT_LIBRARIES=/dev/sglram/m:/dev/sglram/e',
            'SWEET EXCEPTION observer installed process=backboardd limit=16 original_chain=1',
            'SWEET METAL PROBES scheduled process=backboardd constructor_waited=0 background=1',
            'SWEET GLES SUITE END result=0 tests=8 state=2 original_tests=1',
            'SWEET METAL PROBES begin process=backboardd GLES_state=2 full_suite=1 background=1']
        lines += ['SWEET '+s+' END result=0' for s in ('METAL RESOURCE','METAL QUEUE','AIR COMPUTE','AIR RENDER','AIR CONSTANTS','AIR XR','METAL SURFACE','METAL MIPS')]
        lines += ['SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1','SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(lines)
    def test_complete_order(self):self.assertEqual(verify(self.sample(),self.token)['probe_suites'],1)
    def test_bad_order_target_missing_or_failure_rejected(self):
        text=self.sample();ready='SWEET GLES SUITE END result=0 tests=8 state=2 original_tests=1'
        for bad in (text.replace(ready,'').replace('SWEET METAL PROBES END process=backboardd',ready+'\nSWEET METAL PROBES END process=backboardd'),
            text.replace('state=2','state=1'),text.replace('METAL MIPS END result=0','METAL MIPS END result=1'),text.replace('AIR XR END result=0',''),
            text.replace('process=backboardd','process=AegirPoster'),text.replace('constructor_waited=0','constructor_waited=1')):
            with self.assertRaises(AssertionError):verify(bad,self.token)
    def test_injection_targets(self):
        original='sweetenv=backboardd:DYLD_INSERT_LIBRARIES=/dev/sglram/m,SWEETGL_TEST=1,SWEETGL_TRACE=1;SpringBoard:DYLD_INSERT_LIBRARIES=/dev/sglram/g'
        changed=observer_environment(original,'backboardd');self.assertIn('/dev/sglram/m:/dev/sglram/e',changed);self.assertNotIn('AegirPoster:',changed)
        self.assertIn('AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e',observer_environment(original,'AegirPoster'))
        with self.assertRaises(AssertionError):observer_environment(original,'other-process')
if __name__=='__main__':unittest.main()
