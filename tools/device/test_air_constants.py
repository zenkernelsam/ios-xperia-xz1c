"""Independent constant sums and fault replay for native GPU evidence."""
import unittest
from air_constants_evidence import expected_hash,verify
class ConstantsEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_bytes':2200,'library_sha256':'e'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,
            'SWEET AIR CONSTANTS begin library_bytes=2200 standard_MTLB=1 native_constant_values=1 original_buffer=1',
            'SWEET AIR CONSTANTS input library_sha256='+self.fixture['library_sha256'],
            'SWEET AIR CONSTANTS rejection required=1 missing_error=1 wrong_type=1 native_values_class=MTLFunctionConstantValuesInternal valid=1']
        for phase,(step,total) in enumerate(((13,13),(29,42),(37,79))):
            rows.extend(['SWEET AIR MESA compiled stage=compute program=1 buffer_mask=1 write_mask=1 source_bytes=1820 source_hash='+str(phase+1)*16+' linked=1 robust=1',
                'SWEET AIR MESA binding index=0 handle=1 gpu=0x40000000 cpu=0x110000000 offset=0 bytes=320 logical=320 original_pages=1',
                'SWEET AIR MESA dispatch groups=1,1,1 local=64,1,1 result=0 sequence='+str(phase+1)+' original_buffers=1',
                f'SWEET AIR CONSTANTS phase={phase} step={step} total={total} status=4 error=0 sequence={phase+1} operations=1 callback_count={phase+1} checked_words=80 guard_words=16 mismatches=0 hash={expected_hash(phase)} frozen_after_reset=1 valid=1'])
        rows.extend(['SWEET AIR CONSTANTS END result=0 phases=3 invocations=192 checked_words=240 guard_checks=48 final_allocated=0 standard_Metal=1','SWEET LOOP ACK FASTBOOT token='+self.token]);return '\n'.join(rows)
    def test_current_proof(self):
        r=verify(self.sample(),self.token,self.fixture);self.assertEqual((r['complete_samples'],r['incomplete_samples']),(1,0))
        self.assertEqual(len({expected_hash(i) for i in range(3)}),3)
    def test_hash_type_reset_owner_and_fence_rejected(self):
        sample=self.sample()
        for bad in (sample.replace(expected_hash(2),'0'*16),sample.replace('wrong_type=1','wrong_type=0'),sample.replace('frozen_after_reset=1','frozen_after_reset=0'),
            sample.replace('final_allocated=0','final_allocated=16384'),sample.replace('sequence=3','sequence=2'),sample.replace('original_pages=1','original_pages=0'),
            sample.replace('step=29','step=99'),sample.replace('total=79','total=77')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_other_boot(self):
        with self.assertRaises(ValueError):verify(self.sample(),'f'*16,self.fixture)
if __name__=='__main__':unittest.main()
