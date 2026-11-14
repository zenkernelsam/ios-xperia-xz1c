import unittest
from air_narrow_evidence import verify,values
class NarrowEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'7'*64}
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET AIR NARROW begin bits=8,16 signs=signed,unsigned lanes=1,2 phases=2 invocations=64 original_buffers=3',
            'SWEET AIR NARROW input library_sha256='+self.fixture['library_sha256']]
        for phase in (0,1):
            for b,(bytes_,logical) in enumerate(((3136,3200),(320,384),(192,256))):lines.append(f'SWEET AIR MESA binding index={b} handle={b+1} gpu=0x40010000 cpu=0x100010000 offset=64 bytes={bytes_} logical={logical} original_pages=1')
            lines.append(f'SWEET AIR NARROW phase={phase} sequence={phase+1} operations=1 status=4 error=0 checked_bytes=49152 mismatches=0 valid=1')
            for b,h in enumerate(values(phase)):lines.append(f'SWEET AIR NARROW buffer phase={phase} index={b} bytes=16384 hash={h} original_pages=1')
        lines+=['SWEET AIR NARROW END result=0 phases=2 checked_bytes=98304 final_allocated=0 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token];return '\n'.join(lines)
    def test_real_contract(self):self.assertEqual(verify(self.sample(),self.token,self.fixture)['complete_samples'],1)
    def test_wrong_sign_guard_fence_input_rejected(self):
        text=self.sample()
        for bad in (text.replace(values(0)[0],'0'*16),text.replace(values(1)[1],'0'*16),text.replace('sequence=2','sequence=1'),text.replace('final_allocated=0','final_allocated=16384'),text.replace('offset=64','offset=0')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
if __name__=='__main__':unittest.main()
