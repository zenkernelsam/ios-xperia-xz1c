"""Fault replay for real Metal/AIR kernel data, preserved buffer and fences."""
import unittest
from air_compute_evidence import verify,expected_hash

class AIRComputeTests(unittest.TestCase):
    def fixture(self):
        token='0123456789abcdef';fixture={'library_bytes':2722,'library_sha256':'a'*64}
        rows=['SWEET LOOP LIVE proto=1 token='+token,
            'SWEET AIR COMPUTE begin library_bytes=2722 standard_MTLB=1 standard_compute_encoder=1 original_buffer=1',
            'SWEET AIR COMPUTE input library_sha256='+fixture['library_sha256'],
            'SWEET AIR MESA compiled stage=compute program=1 buffer_mask=1 write_mask=1 source_bytes=1820 source_hash=0123456789abcdef linked=1 robust=1']
        for phase,sequence in enumerate((33,44)):
            rows+=['SWEET AIR MESA binding index=0 handle=8 gpu=0x40020000 cpu=0x110000000 offset=0 bytes=320 logical=320 original_pages=1',
                'SWEET AIR MESA dispatch groups=1,1,1 local=64,1,1 result=0 sequence='+str(sequence)+' original_buffers=1',
                'SWEET AIR COMPUTE phase='+str(phase)+' status=4 error=0 sequence='+str(sequence)+' operations=1 callback_count='+str(phase+1)+
                ' checked_words=80 guard_words=16 mismatches=0 hash='+expected_hash(phase)+' same_original_pages=1 valid=1']
        rows+=['SWEET AIR COMPUTE END result=0 phases=2 kernel_invocations=128 checked_words=160 guard_checks=32 final_allocated=0 standard_Metal=1',
            'SWEET LOOP ACK FASTBOOT token='+token]
        return token,fixture,'\n'.join(rows)
    def test_actual_kernel_and_shared_bytes(self):
        token,fixture,text=self.fixture();proof=verify(text,token,fixture);self.assertEqual(proof['records'][0]['sequence'],[33,44])
        for changed in (text.replace('library_bytes=2722','library_bytes=2723'),text.replace('a'*64,'b'*64),
                        text.replace('robust=1','robust=0'),text.replace('original_pages=1','original_pages=0'),
                        text.replace('checked_words=80','checked_words=64'),text.replace(expected_hash(0),'0'*16),
                        text.replace('guard_words=16','guard_words=0'),text.replace('status=4','status=5'),
                        text.replace('sequence=44','sequence=33'),text.replace('final_allocated=0','final_allocated=16384')):
            with self.assertRaises(AssertionError):verify(changed,token,fixture)
    def test_original_pages_cannot_change_between_phases(self):
        token,fixture,text=self.fixture()
        changed=text.replace('sequence=44 original_buffers=1','sequence=45 original_buffers=1')
        with self.assertRaises(AssertionError):verify(changed,token,fixture)
        needle='SWEET AIR MESA binding index=0 handle=8 gpu=0x40020000 cpu=0x110000000'
        index=text.rindex(needle);changed=text[:index]+text[index:].replace('cpu=0x110000000','cpu=0x110004000',1)
        with self.assertRaises(AssertionError):verify(changed,token,fixture)
        with self.assertRaises(ValueError):verify(text,'0'*16,fixture)

if __name__=='__main__':unittest.main()
