"""Missing transient bytes or shared queue pages cannot admit Memoryless."""
import unittest
from memoryless_evidence import BEGIN, SLOTS, OPERATIONS, expected, verify


class MemorylessEvidence(unittest.TestCase):
    token='0123456789abcdef'; fixture={'library_sha256':'a'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token, BEGIN,
              'SWEET MEMORYLESS INPUT fixture_sha256='+self.fixture['library_sha256'],
              'SWEET MEMORYLESS PUBLIC valid=1 allocated=0 depth=0000000100100000 stencil=0000000100200000 color=0000000100300000 backing_exposed=0']
        for phase in range(8):
            if phase==6:rows.append('SWEET MEMORYLESS ISOLATION valid=1 two_queues=1 encoders_materialized_before_commit=1 different_depth_and_stencil_pages=1')
            for slot in SLOTS[phase]:
                handle=phase*10+slot+1;gpu=0x40000000+handle*16384
                public={0:'0000000100300000',8:'0000000100100000',9:'0000000100200000'}[slot]
                size,pitch,format=(704,64,253) if slot==9 else (1280,128,252 if slot==8 else 80)
                rows.append(f'SWEET MEMORYLESS PAGE phase={phase} slot={slot} pass={phase+1} public={public} handle={handle} GPU={hex(gpu)} bytes={size} pitch={pitch} offset=64 format={format}')
            for operation in range(OPERATIONS[phase]):
                for slot in SLOTS[phase]:
                    if slot==0:
                        handle=phase*10+slot+1;gpu=0x40000000+handle*16384
                        rows += [f'SWEET AIR RENDER texture handle={handle} width=17 height=9 pitch=128 format=80 offset=64 gpu={hex(gpu)} original_pages=1',
                                 f'SWEET METAL MRT retired index=0 handle={handle} format=80 result=0 sequence={phase*10+operation+1} original_pages=1']
                        continue
                    name='DEPTH' if slot==8 else 'STENCIL';handle=phase*10+slot+1;gpu=0x40000000+handle*16384;pitch=128 if slot==8 else 64
                    rows += [f'SWEET METAL {name} import handle={handle} width=17 height=9 pitch={pitch} offset=64 GPU={hex(gpu)} original_pages=1',
                             f'SWEET METAL {name} retired handle={handle} result=0 sequence={phase*10+operation+1} original_pages=1']
            checked=1280+(1280 if 8 in SLOTS[phase] else 0)+(704 if 9 in SLOTS[phase] else 0)
            c,d,s=expected(phase)
            rows += [f'SWEET MEMORYLESS PHASE index={phase} valid=1 sequence={phase*10+9} operations={OPERATIONS[phase]} checked_bytes={checked} mismatches=0 original_pass_pages=1',
                     f'SWEET MEMORYLESS HASH index={phase} color={c} depth={d} stencil={s}']
        rows += ['SWEET MEMORYLESS END result=0 phases=8 checked_bytes=19456 guard_bytes=10888 final_allocated=0 GPU_only=1 transient_pages=1',
                 'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)+'\n'
    def test_original_recipe_and_independent_pass_storage(self):
        result=verify(self.sample(),self.token,self.fixture)
        self.assertEqual(result['checked_bytes'],19456);self.assertEqual(len(result['original_pass_pages']),11)
        self.assertNotEqual(expected(0)[0],expected(1)[0]);self.assertEqual(expected(4)[1],'0'*16)
        self.assertNotEqual(expected(2)[2],expected(3)[2])
    def test_missing_wrong_page_format_address_fence_or_byte_rejected(self):
        text=self.sample();c,d,s=expected(3)
        for bad in (text.replace('phases=8 checked_bytes','phases=7 checked_bytes'),text.replace('final_allocated=0','final_allocated=16384'),
                    text.replace('valid=1 allocated=0','valid=0 allocated=0'),text.replace('phase=0 slot=8 pass=1','phase=0 slot=8 pass=0'),
                    text.replace('GPU=0x40024000','GPU=0x40024040'),text.replace('bytes=704 pitch=64','bytes=768 pitch=64'),
                    text.replace('index=1 valid=1 sequence=19','index=1 valid=1 sequence=9'),
                    text.replace('index=3 color='+c+' depth='+d+' stencil='+s,'index=3 color='+c+' depth='+d+' stencil='+'0'*16),
                    text.replace('original_pages=1','original_pages=0',1),text.replace('public=0000000100100000','public=0000000100200000',1),
                    text.replace('two_queues=1','two_queues=0'),text.replace('SWEET MEMORYLESS HASH index=1','SWEET MEMORYLESS UNKNOWN index=1')):
            with self.subTest(changed=bad!=text),self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_concurrent_pages_must_have_distinct_addresses_and_handles(self):
        text=self.sample()
        for bad in (text.replace('phase=7 slot=8 pass=8','phase=7 slot=8 pass=7'),
                    text.replace('handle=79','handle=69'),text.replace('GPU=0x4013c000','GPU=0x40114000')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
        line='SWEET AIR RENDER texture handle=41 width=17 height=9 pitch=128 format=80 offset=64 gpu=0x400a4000 original_pages=1\n'
        with self.assertRaises(AssertionError):verify(text.replace(line,''),self.token,self.fixture)
    def test_post_ACK_end_cannot_complete_missing_scope(self):
        text=self.sample();line='SWEET MEMORYLESS END result=0 phases=8 checked_bytes=19456 guard_bytes=10888 final_allocated=0 GPU_only=1 transient_pages=1\n'
        with self.assertRaises(AssertionError):verify(text.replace(line,'')+line,self.token,self.fixture)


if __name__=='__main__':unittest.main()
