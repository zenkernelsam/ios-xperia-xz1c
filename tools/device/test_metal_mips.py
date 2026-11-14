import unittest
from metal_mip_evidence import layouts,raw_expected,output_colour,output_expected,verify
class MipEvidence(unittest.TestCase):
    token='0123456789abcdef'
    def sample(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET METAL MIPS begin formats=70,80,81,553 levels=6 generations=2 samples=3 original_pages=1'];seq=0
        for f in (70,80,81,553):
            for phase in (0,1):
                codes=[188]*5 if f==81 else [0]*5
                for level,(w,h,_,_) in enumerate(layouts(f)[1:],1):
                    seq+=2;lines.append(f'SWEET METAL MIPMAP level={level} src_handle=7 dst_handle=7 width={w} height={h} result=0 sequence={seq} original_pages=1')
                lines.append(f'SWEET METAL MIPS generation format={f} phase={phase} sequence={seq} operations=5 mismatches=0 raw_hash={raw_expected(f,phase,codes)} valid=1')
                lines.append(f'SWEET METAL MIPS codes format={f} phase={phase} '+ ' '.join(f'level{n+1}={c}' for n,c in enumerate(codes))+f' tolerance={int(f==81)}')
                for s in range(3):
                    seq+=7;colour=output_colour(f,phase,s,codes);hashed=output_expected(f,phase,s,codes,colour)
                    lines.append('SWEET METAL MIPMAP sample_chain levels=6 width=32 height=16 shadow_bytes=10928 result=0 GPU_only=1 original_levels=1')
                    lines.append(f'SWEET METAL MIPS sample format={f} phase={phase} sample={s} sequence={seq} operations=1 mismatches=0 max_error=0 output_hash={hashed} valid=1')
                    lines.append(f'SWEET METAL MIPS colour format={f} phase={phase} sample={s} b={colour[0]} g={colour[1]} r={colour[2]} a={colour[3]}')
        lines+=['SWEET METAL MIPS END result=0 formats=4 levels=6 generations=8 samples=24 checked_bytes=917504 final_allocated=0 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token];return '\n'.join(lines)
    def test_actual_contract(self):self.assertEqual(verify(self.sample(),self.token)['complete_samples'],1)
    def test_hash_padding_levels_fences_colour_rejected(self):
        text=self.sample()
        for bad in (text.replace('width=16 height=8','width=8 height=8'),text.replace('level1=188','level1=128'),text.replace('operations=5','operations=4'),
            text.replace('raw_hash='+raw_expected(70,0,[0]*5),'raw_hash='+'0'*16),text.replace('GPU_only=1 original_levels=1','GPU_only=0 original_levels=1'),
            text.replace('final_allocated=0','final_allocated=16384'),text.replace('src_handle=7 dst_handle=7','src_handle=8 dst_handle=7')):
            with self.assertRaises(AssertionError):verify(bad,self.token)
if __name__=='__main__':unittest.main()
