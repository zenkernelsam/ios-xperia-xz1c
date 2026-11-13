"""Reject mismatched current-boot render hashes, views, fences and lifetime."""
import unittest
from air_render_evidence import expected,verify

class RenderEvidence(unittest.TestCase):
    token='0123456789abcdef'
    fixture={'library_bytes':3657,'library_sha256':'c'*64}
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,
            'SWEET AIR RENDER begin library_bytes=3657 standard_MTLB=1 standard_render_encoder=1 original_textures=1',
            'SWEET AIR RENDER contract target_bytes=3776 sample_bytes=512 target_pixels=627 target_offset=64 target_pitch=192 sample_offset=128 sample_pitch=64',
            'SWEET AIR RENDER input library_sha256='+self.fixture['library_sha256']]
        for program in (1,2):lines.append('SWEET AIR RENDER compiled program='+str(program)+' vertex_mask=1 fragment_mask=0 format=80 vertex_hash='+'a'*16+' fragment_hash='+'b'*16+' linked=1 robust=1')
        for phase in range(4):
            lines.append('SWEET AIR RENDER texture handle=1 width=33 height=19 pitch=192 format=80 offset=64 gpu=0x40000000 original_pages=1')
            if phase==0:lines.append('SWEET AIR RENDER bounded_clear width=33 height=19 shader_draw=1 result=0')
            else:
                handle,bytes_=(3,48) if phase==1 else (4,96)
                lines.append(f'SWEET AIR RENDER vertex_words index=0 handle={handle} gpu=0x40020000 offset=0 bytes={bytes_} logical={bytes_} integer_texture=1 guarded=1 original_pages=1')
            if phase>=2:lines.append('SWEET AIR RENDER texture handle=2 width=7 height=5 pitch=64 format=70 offset=128 gpu=0x40010000 original_pages=1')
            if phase:lines.append('SWEET AIR RENDER draw width=33 height=19 primitive=3 first=0 count=3 instances=1 target_handle=1 result=0 sequence='+str(phase+1)+' original_pages=1')
            target,source=expected(phase)
            lines.append(f'SWEET AIR RENDER phase={phase} status=4 error=0 sequence={phase+1} operations=1 callback_count={phase+1} mismatches=0 read_mismatches=0 target_hash={target} sample_hash={source} original_pages=1 valid=1')
        lines.extend(['SWEET AIR RENDER END result=0 phases=4 draw_ops=3 clear_ops=1 checked_target_bytes=15104 checked_sample_bytes=2048 guard_bytes=6560 final_allocated=0 standard_Metal=1','SWEET LOOP ACK FASTBOOT token='+self.token])
        return '\n'.join(lines)
    def test_full_proof(self):
        report=verify(self.sample(),self.token,self.fixture)
        self.assertEqual((report['complete_samples'],report['incomplete_samples']),(1,0))
        self.assertEqual(report['records'][0]['checked_guard_bytes'],6560)
        self.assertEqual(len({expected(phase)[0] for phase in range(4)}),4)
    def test_corrupted_pixel_and_view_rejected(self):
        sample=self.sample()
        for wrong in (sample.replace(expected(2)[0],'0'*16),sample.replace('pitch=192','pitch=128'),
            sample.replace('offset=128','offset=64'),sample.replace('original_pages=1 valid=1','original_pages=0 valid=1'),
            sample.replace('final_allocated=0','final_allocated=16384'),sample.replace('result=0 phases=4','result=1 phases=4'),
            sample.replace('callback_count=4','callback_count=3'),sample.replace('guarded=1','guarded=0')):
            with self.subTest(wrong=wrong):
                with self.assertRaises(AssertionError):verify(wrong,self.token,self.fixture)
    def test_repeated_fence_rejected(self):
        with self.assertRaises(AssertionError):verify(self.sample().replace('sequence=4','sequence=3'),self.token,self.fixture)
    def test_other_boot_rejected(self):
        with self.assertRaises(ValueError):verify(self.sample(),'fedcba9876543210',self.fixture)
    def test_partial_sample_not_complete(self):
        text='\n'.join(line for line in self.sample().splitlines() if 'SWEET AIR RENDER phase=3' not in line)
        with self.assertRaises(AssertionError):verify(text,self.token,self.fixture)

if __name__=='__main__':unittest.main()
