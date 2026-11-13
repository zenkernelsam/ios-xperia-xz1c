"""Fault replay for metadata identity, exact method types and complete samples."""
import unittest
from metal_abi_evidence import SELECTORS,verify

class MetalABITests(unittest.TestCase):
    def fixture(self):
        token='0123456789abcdef'
        begin='SWEET METAL ABI begin class=_MTLDevice present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1 device_protocol=1 buffer_protocol=1 queue_protocol=1 command_protocol=1 metadata_only=1\n'
        rows=[]
        for i,(owner,selector) in enumerate(SELECTORS):
            type_=['@16@0:8','@20@0:8I16','v16@0:8','v16@0:8','Q16@0:8'][i] if i<5 else '@16@0:8'
            rows.append('SWEET METAL ABI method index='+str(i)+' owner='+owner+' selector='+selector+' present=1 types_hex='+type_.encode().hex()+' metadata_only=1')
        block=begin+'\n'.join(rows)+'\nSWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0\n'
        return token,block,'SWEET LOOP LIVE proto=1 token='+token+'\n'+block+'SWEET LOOP ACK FASTBOOT token='+token
    def test_observed_class_and_32bit_initializer(self):
        token,_,text=self.fixture();report=verify(text,token)
        self.assertEqual(report['methods'][1]['types'],'@20@0:8I16')
        for changed in (text.replace('limits_bytes=264','limits_bytes=256'),text.replace('limits_bounded=1','limits_bounded=0'),
                        text.replace('@20@0:8I16'.encode().hex(),'@24@0:8Q16'.encode().hex()),
                        text.replace('instances_created=0','instances_created=1'),text.replace('gpu_operations=0','gpu_operations=1'),
                        text.replace('class=_MTLDevice','class=FakeMetalDevice'),text.replace('device_protocol=1','device_protocol=0')):
            with self.assertRaises(AssertionError):verify(changed,token)
    def test_lost_method_not_filled_from_other_sample(self):
        token,block,text=self.fixture();missing='\n'.join(s for s in block.splitlines() if 'index=8 ' not in s)+'\n'
        report=verify(text.replace(block,missing+block),token)
        self.assertEqual(report['complete_samples'],1);self.assertEqual(report['incomplete_samples'],1)
        with self.assertRaises(AssertionError):verify(text.replace(block,missing),token)
    def test_token_and_method_order(self):
        token,_,text=self.fixture()
        for changed in (text.replace('index=8 ','index=7 '),text.replace('owner=buffer selector=contents','owner=device selector=contents')):
            with self.assertRaises(AssertionError):verify(changed,token)
        with self.assertRaises(ValueError):verify(text,'0'*16)
    def test_actual_abstract_feature_getter_absence_is_preserved(self):
        token,_,text=self.fixture()
        marker='owner=device_base selector=featureProfile present=1 types_hex='+b'Q16@0:8'.hex()
        changed=text.replace(marker,'owner=device_base selector=featureProfile present=0 types_hex=none')
        report=verify(changed,token)
        self.assertTrue(report['base_featureProfile_implementation_missing'])
        self.assertIsNone(report['methods'][4]['types'])

if __name__=='__main__':unittest.main()
