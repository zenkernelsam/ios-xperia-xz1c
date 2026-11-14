import unittest
from metal_discovery_evidence import verify
class DiscoveryEvidence(unittest.TestCase):
    def test_actual_registration_and_negative_metadata(self):
        token='0123456789abcdef'
        text='\n'.join(['SWEET LOOP LIVE proto=1 token='+token,
            'SWEET METAL DISCOVERY begin process=AegirPoster original_MTLAddDevice=1 original_default_device=1 native_GPU=1',
            'SWEET METAL DISCOVERY device_nil=0 device_class=SweetMTLDevice registry=0000000100000021 SPI=1 real_factory=1',
            'SWEET METAL DISCOVERY END result=0 same_original_default=1 original_API=1 GPU_work_preverified=1',
            'SWEET LOOP ACK FASTBOOT token='+token])
        self.assertEqual(verify(text,token)['complete_samples'],1)
        for bad in (text.replace('SPI=1','SPI=0'),text.replace('same_original_default=1','same_original_default=0'),
            text.replace('registry=0000000100000021','registry=0000000000000000'),text.replace('device_nil=0','device_nil=1'),text.replace('result=0','result=1')):
            with self.assertRaises(AssertionError):verify(bad,token)
if __name__=='__main__':unittest.main()
