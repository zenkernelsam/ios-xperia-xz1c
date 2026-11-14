import unittest,struct
from air_half_evidence import expected,graphics,verify
from air_render_evidence import fnv
class HalfEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'6'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET AIR HALF begin phases=2 invocations=64 raw16_storage=1 result_rounding=1 graphics=1 original_buffers=1',
            'SWEET AIR HALF input library_sha256='+self.fixture['library_sha256']]
        for phase in (0,1):
            output,input_,full,nans=expected(phase);rows.append(f'SWEET AIR HALF phase={phase} sequence={phase+1} operations=1 status=4 error=0 checked_bytes=49152 mismatches=0 valid=1')
            rows.append(f'SWEET AIR HALF normalized phase={phase} output_hash={fnv(output)} nan_words={len(nans)} finite_exact=1 NaN_class=1')
            raw=bytearray(output)
            for word in nans:
                raw[64+word*2:66+word*2]=struct.pack('<H',0xfe01);rows.append(f'SWEET AIR HALF NaN phase={phase} word={word} bits=fe01')
            for b,p in enumerate((raw,input_,full)):rows.append(f'SWEET AIR HALF buffer phase={phase} index={b} raw_hash={fnv(p)} original_pages=1')
        target,vertex=graphics();rows += ['SWEET AIR HALF graphics sequence=3 operations=1 status=4 error=0 checked_bytes=32768 pixels=153 mismatches=0 valid=1',
            f'SWEET AIR HALF graphics_hash target={target} vertices={vertex} original_pages=1',
            'SWEET AIR HALF END result=0 checked_bytes=131072 phases=2 graphics=1 final_allocated=0 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)
    def test_finite_bits_nan_payload_and_graphics(self):self.assertEqual(verify(self.sample(),self.token,self.fixture)['complete_samples'],1)
    def test_bit_guard_nan_and_fence_faults(self):
        text=self.sample()
        for bad in (text.replace('bits=fe01','bits=7c00'),text.replace('sequence=3','sequence=2'),text.replace('final_allocated=0','final_allocated=16384'),
            text.replace(graphics()[0],'0'*16),text.replace(fnv(expected(0)[1]),'0'*16)):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
if __name__=='__main__':unittest.main()
