"""Fault records and incorrect byte recipes cannot qualify depth support."""
import unittest
from depth32_evidence import verify,expected,BEGIN


class DepthEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'a'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,BEGIN,'SWEET DEPTH32 INPUT fixture_sha256='+self.fixture['library_sha256']]
        for phase in range(29):
            rows.append('SWEET METAL DEPTH import handle=1 width=17 height=9 pitch=128 offset=64 GPU=0x40004000 original_pages=1')
            if phase<25 or phase==28:rows.append('SWEET METAL DEPTH clear handle=1 width=17 height=9 shader_draw=1 result=0 original_pages=1')
            rows.append(f'SWEET METAL DEPTH retired handle=1 result=0 sequence={phase+1} original_pages=1')
            c,d=expected(phase);rows+=[f'SWEET DEPTH32 PHASE index={phase} valid=1 sequence={phase+1} operations=1 mismatches=0 read_mismatches=0',
                                     f'SWEET DEPTH32 HASH index={phase} color={c} depth={d} original_pages=1']
        rows+=['SWEET DEPTH32 END result=0 phases=29 checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=0 GPU_only=1',
               'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)+'\n'
    def test_recipe_and_complete_scope(self):
        r=verify(self.sample(),self.token,self.fixture);self.assertEqual(r['phases'],29)
        self.assertEqual(expected(4)[0],expected(25)[0])
        self.assertNotEqual(expected(24),expected(25));self.assertNotEqual(expected(27),expected(28))
    def test_faults_rejected(self):
        text=self.sample()
        for bad in (text.replace('index=1 valid=1 sequence=2','index=1 valid=1 sequence=1',1),text.replace('valid=1','valid=0',1),text.replace('mismatches=0','mismatches=1',1),
                    text.replace('original_pages=1','original_pages=0',1),text.replace('final_allocated=0','final_allocated=16384'),
                    text.replace('SWEET METAL DEPTH clear handle=1 width=17 height=9 shader_draw=1 result=0 original_pages=1\n','',1),text.replace('phases=29 checked_bytes','phases=28 checked_bytes'),
                    text.replace('fixture_sha256='+'a'*64,'fixture_sha256='+'b'*64),text.replace('index=7 color','index=8 color')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_missing_stage_after_ACK_does_not_complete(self):
        text=self.sample();line='SWEET DEPTH32 END result=0 phases=29 checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=0 GPU_only=1\n'
        with self.assertRaises(AssertionError):verify(text.replace(line,'')+line,self.token,self.fixture)
    def test_actual_import_handle_offset_and_retirement_cannot_differ(self):
        text=self.sample()
        for bad in (text.replace('GPU=0x40004000','GPU=0x40004040',1),text.replace('offset=64 GPU','offset=0 GPU',1),
                    text.replace('import handle=1','import handle=2',1),text.replace('retired handle=1 result=0','retired handle=1 result=-5',1),
                    text.replace('sequence=1 original_pages=1','sequence=999 original_pages=1',1)):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)


if __name__=='__main__':unittest.main()
