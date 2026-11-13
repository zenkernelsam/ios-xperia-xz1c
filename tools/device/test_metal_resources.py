"""Fault replay for real resource backing and retained object lifetime evidence."""
import unittest
from metal_resource_evidence import verify,expected_hash

class ResourceTests(unittest.TestCase):
    def fixture(self):
        token='0123456789abcdef';rows=['SWEET LOOP LIVE proto=1 token='+token,
            'SWEET METAL RESOURCE begin custom_factory=1 typed_objects=1 kernel_backing=SweetScatter gpu_submit=0']
        for i,(length,span) in enumerate(zip((432,16384,16385),(16384,16384,32768))):
            rows.append('SWEET METAL RESOURCE view index='+str(i)+' length='+str(length)+' allocated='+str(span)+
                ' gpu='+hex(0x40000000+i*65536)+' cpu='+hex(0x110000000+i*65536)+
                ' cache=0 storage=0 checked='+str(length)+' mismatches=0 hash='+expected_hash(i,length)+' owner_match=1 valid=1')
        rows+=['SWEET METAL RESOURCE lifetime owner_released=1 held_owner=1 current=65536 valid=1',
            'SWEET METAL RESOURCE END result=0 buffers=3 bytes=33201 span=65536 final_allocated=0 gpu_submit=0',
            'SWEET LOOP ACK FASTBOOT token='+token]
        return token,'\n'.join(rows)
    def test_source_and_hash_and_lifetime(self):
        token,text=self.fixture();self.assertEqual(verify(text,token)['final_allocated_bytes'],0)
        for changed in (text.replace('length=432','length=433'),text.replace('allocated=32768','allocated=16384'),
                        text.replace('gpu=0x40000000','gpu=0x80000000'),text.replace('cpu=0x110000000','cpu=0x110000001'),
                        text.replace('mismatches=0','mismatches=1'),text.replace(expected_hash(0,432),'0'*16),
                        text.replace('final_allocated=0','final_allocated=16384'),text.replace('held_owner=1','held_owner=0'),
                        text.replace('gpu_submit=0','gpu_submit=1')):
            with self.assertRaises(AssertionError):verify(changed,token)
    def test_no_alias_backing_or_foreign_boot(self):
        token,text=self.fixture()
        with self.assertRaises(AssertionError):verify(text.replace('gpu=0x40010000','gpu=0x40000000'),token)
        with self.assertRaises(ValueError):verify(text,'0'*16)

if __name__=='__main__':unittest.main()
