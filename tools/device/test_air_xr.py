import unittest
from air_xr_evidence import expected,verify
class XREvidence(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET AIR XR begin formats=552,553,554,555 standard_texture_render=1 raw_code_storage=1 GPU_sample_decode=1'];sequence=0
        for format in range(552,556):
            for phase in range(3):
                sequence+=1;raw,out,count=expected(format,phase)
                if phase==2:lines.append(f'SWEET AIR XR sample_decode format={format} handle={format-550} width=17 height=9 result=0 GPU_only=1 original_source_pages=1 float_shadow=1')
                lines.append(f'SWEET AIR XR format={format} phase={phase} status=4 error=0 sequence={sequence} operations=1 raw_bytes={count} output_bytes=1344 pixels=153 mismatches=0 raw_hash={raw} output_hash={out} original_pages=1 valid=1')
        lines.extend(['SWEET AIR XR END result=0 formats=4 phases=12 checked_raw_output_bytes=34944 final_allocated=0 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token]);return '\n'.join(lines)
    def test_actual_formats(self):self.assertEqual(verify(self.sample(),self.token)['complete_samples'],1)
    def test_code_hash_guard_decode_and_fence_rejected(self):
        sample=self.sample()
        for wrong in (sample.replace(expected(553,1)[0],'0'*16),sample.replace('sequence=12','sequence=11'),sample.replace('raw_bytes=1856','raw_bytes=1280'),
            sample.replace('GPU_only=1 original_source_pages=1','GPU_only=0 original_source_pages=1'),sample.replace('final_allocated=0','final_allocated=16384')):
            with self.assertRaises(AssertionError):verify(wrong,self.token)
if __name__=='__main__':unittest.main()
