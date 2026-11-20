"""Missing stages, masks/bytes or original plane identity must refuse Stencil8."""
import unittest
from stencil8_evidence import BEGIN,expected,verify


class StencilEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'a'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,BEGIN,'SWEET STENCIL8 INPUT fixture_sha256='+self.fixture['library_sha256']]
        for phase in range(99):
            rows.append('SWEET METAL STENCIL import handle=1 width=17 height=9 pitch=64 offset=64 GPU=0x40004000 original_pages=1 separate_z32s8=1')
            if phase!=98:rows.append('SWEET METAL STENCIL clear handle=1 width=17 height=9 shader_draw=1 result=0 original_pages=1')
            if phase!=96:rows.append('SWEET METAL DEPTH import handle=2 width=17 height=9 pitch=128 offset=64 GPU=0x40008000 original_pages=1')
            rows.append(f'SWEET METAL STENCIL retired handle=1 result=0 sequence={phase+1} original_pages=1')
            c,d,s=expected(phase);rows += [f'SWEET STENCIL8 PHASE index={phase} valid=1 sequence={phase+1} operations=1 mismatches=0 read_mismatches=0',
                                         f'SWEET STENCIL8 HASH index={phase} color={c} depth={d} stencil={s} original_pages=1']
        rows+=['SWEET STENCIL8 END result=0 phases=99 checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=0 GPU_only=1',
               'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)+'\n'
    def test_complete_original_planes_and_independent_recipe(self):
        r=verify(self.sample(),self.token,self.fixture);self.assertEqual(r['phases'],99)
        self.assertEqual(r['original_depth_handle'],2);self.assertEqual(r['original_stencil_handle'],1)
        self.assertEqual(expected(96)[:2],expected(97)[:2]);self.assertNotEqual(expected(96)[2],expected(97)[2])
        self.assertEqual(expected(97)[2],expected(98)[2]);self.assertNotEqual(expected(97)[1],expected(98)[1])
    def test_missing_corrupt_or_foreign_plane_rejected(self):
        text=self.sample()
        bad=(text.replace('index=1 valid=1 sequence=2','index=1 valid=1 sequence=1',1),
             text.replace('original_pages=1 separate_z32s8=1','original_pages=0 separate_z32s8=1',1),
             text.replace('GPU=0x40004000','GPU=0x40004040',1),text.replace('import handle=2','import handle=1',1),
             text.replace('retired handle=1 result=0','retired handle=1 result=-5',1),
             text.replace('valid=1 sequence=13','valid=0 sequence=13',1),text.replace('mismatches=0','mismatches=1',1),
             text.replace('SWEET METAL STENCIL clear handle=1 width=17 height=9 shader_draw=1 result=0 original_pages=1\n','',1),
             text.replace('fixture_sha256='+'a'*64,'fixture_sha256='+'b'*64),text.replace('phases=99 checked_bytes','phases=98 checked_bytes'),
             text.replace('index=33 color','index=34 color'),text.replace('final_allocated=0','final_allocated=16384'))
        for b in bad:
            with self.subTest(changed=b!=text),self.assertRaises(AssertionError):verify(b,self.token,self.fixture)
    def test_wrong_stencil_expected_bytes_rejected(self):
        text=self.sample();c,d,s=expected(59)
        with self.assertRaises(AssertionError):verify(text.replace('index=59 color='+c+' depth='+d+' stencil='+s,'index=59 color='+c+' depth='+d+' stencil='+'0'*16),self.token,self.fixture)
    def test_exit_period_not_missing_stage_substitute(self):
        text=self.sample();line='SWEET STENCIL8 END result=0 phases=99 checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=0 GPU_only=1\n'
        with self.assertRaises(AssertionError):verify(text.replace(line,'')+line,self.token,self.fixture)


if __name__=='__main__':unittest.main()
