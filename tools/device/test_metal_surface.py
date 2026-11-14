import unittest
from metal_surface_evidence import expected,verify
class SurfaceEvidence(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET METAL SURFACE begin cases=2 phases=3 width=17 height=9 pitch=128 original_IOSurface=1 caller_release=1'];sequence=0
        for offset in (64,128):
            lines.append(f'SWEET METAL SURFACE backing offset={offset} id=23 handle=7 gpu={0x40010000+offset:#x} cpu={0x100010000+offset:#x} bytes=1152 original_origin=1 valid=1')
            for phase in range(3):
                sequence+=1;raw,out=expected(phase)
                if phase==1:lines.append(f'SWEET METAL SURFACE retained offset={offset} caller_unmapped=1 caller_CF_released=1 texture_holds_surface=1')
                lines.append(f'SWEET METAL SURFACE phase offset={offset} phase={phase} sequence={sequence} operations=1 status=4 error=0 mismatches=0 raw_hash={raw} output_hash={out} original_pages=1 valid=1')
        lines+=['SWEET METAL SURFACE END result=0 cases=2 phases=6 checked_bytes=14976 row_padding=6480 final_allocated=0 original_surface_GPU=1','SWEET LOOP ACK FASTBOOT token='+self.token];return '\n'.join(lines)
    def test_original_pages(self):self.assertEqual(verify(self.sample(),self.token)['complete_samples'],1)
    def test_failures_rejected(self):
        text=self.sample()
        for bad in (text.replace(expected(2)[0],'0'*16),text.replace('caller_unmapped=1','caller_unmapped=0'),text.replace('gpu=0x40010040','gpu=0x40010080'),
            text.replace('sequence=6','sequence=5'),text.replace('final_allocated=0','final_allocated=1152'),text.replace('phase=2','phase=1')):
            with self.assertRaises(AssertionError):verify(bad,self.token)
    def test_return_boundary(self):
        failure='SWEET METAL SURFACE END result=1 cases=1 phases=6 checked_bytes=14976 row_padding=6480 final_allocated=0 original_surface_GPU=1'
        sample=self.sample();self.assertEqual(verify(sample+'\n'+failure,self.token)['complete_samples'],1)
        with self.assertRaises(AssertionError):verify(sample.replace('SWEET LOOP ACK FASTBOOT',failure+'\nSWEET LOOP ACK FASTBOOT'),self.token)
if __name__=='__main__':unittest.main()
