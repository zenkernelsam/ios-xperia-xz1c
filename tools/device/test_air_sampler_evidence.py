import unittest
from air_sampler_evidence import expected,expand,verify
from air_render_evidence import fnv


class SamplerEvidenceTests(unittest.TestCase):
    token='0123456789abcdef'
    fixture={'library_sha256':'a'*64}

    def capture(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,
               'SWEET AIR SAMPLER begin phases=3 original_word=807bff0000080a49 literal_states=1 explicit_LOD=1 source_slot=5 external_samplers=0 original_pages=1',
               'SWEET AIR SAMPLER input library_sha256='+self.fixture['library_sha256']]
        for phase in range(3):
            lines.append('SWEET AIR SAMPLER phase=%d sequence=%d operations=1 status=4 error=0 checked_bytes=49152 pixels=627 mismatches=0 original_pages=1 external_samplers=0 valid=1'%(phase,phase+1))
            for buffer,data in enumerate(expected(phase)):
                lines.append('SWEET AIR SAMPLER buffer phase=%d index=%d raw_hash=%s original_pages=1'%(phase,buffer,fnv(data)))
                for offset in range(0,16384,64):
                    lines.append('SWEET AIR SAMPLER data phase=%d buffer=%d offset=%d bytes=64 hex=%s'%(phase,buffer,offset,data[offset:offset+64].hex()))
        lines+=['SWEET AIR SAMPLER END result=0 checked_bytes=147456 phases=3 final_allocated=0 GPU_only=1',
                'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(lines)

    def test_every_buffer_byte(self):
        result=verify(self.capture(),self.token,self.fixture)
        self.assertEqual(result['complete_samples'],1)
        self.assertEqual(result['samples'][0]['bytes_checked'],147456)

    def test_guard_mutation_refused(self):
        text=self.capture().replace('hex='+'a5'*64,'hex=00'+'a5'*63,1)
        with self.assertRaises(AssertionError):verify(text,self.token,self.fixture)

    def test_missing_or_duplicate_record_refused(self):
        text=self.capture();row=next(line for line in text.splitlines() if 'data phase=1 buffer=2 offset=0 ' in line)
        for changed in [text.replace(row,'',1),text.replace(row,row+'\n'+row,1)]:
            with self.assertRaises(AssertionError):verify(changed,self.token,self.fixture)

    def test_rle_and_bounds(self):
        self.assertEqual(expand('SWEET AIR SAMPLER run phase=0 buffer=0 offset=0 bytes=16384 pattern=aabbccdd',0,0),bytes.fromhex('aabbccdd')*4096)
        for text in ['SWEET AIR SAMPLER run phase=0 buffer=0 offset=0 bytes=16385 pattern=00',
                     'SWEET AIR SAMPLER run phase=0 buffer=0 offset=1 bytes=16384 pattern=00']:
            with self.assertRaises(AssertionError):expand(text,0,0)

    def test_wrong_input_or_failed_end_refused(self):
        for text in [self.capture().replace('library_sha256='+'a'*64,'library_sha256='+'b'*64),
                     self.capture().replace('END result=0','END result=1')]:
            with self.assertRaises(AssertionError):verify(text,self.token,self.fixture)


if __name__=='__main__':unittest.main(verbosity=2)
