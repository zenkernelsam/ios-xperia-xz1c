"""USB open observation cannot issue bulk commands or hide its failed phase."""
import unittest
from types import SimpleNamespace
from usb_open_observation import inspect_device


class Util:
    ENDPOINT_TYPE_BULK=2
    def __init__(self, failure=None): self.calls=[]; self.failure=failure
    def get_string(self, device, index):
        self.calls.append('identity')
        if self.failure=='identity': raise OSError(2,'Entity not found')
        return 'BH905ND69G'
    def endpoint_type(self, value): return value&3
    def claim_interface(self, device, index):
        self.calls.append('claim');assert index==0
        if self.failure=='claim': raise OSError(2,'Entity not found')
    def release_interface(self, device, index): self.calls.append('release');assert index==0
    def dispose_resources(self, device): self.calls.append('dispose')


class Device:
    iSerialNumber=1
    def __init__(self, failure=None): self.failure=failure
    def get_active_configuration(self):
        if self.failure=='configuration': raise OSError(2,'Entity not found')
        interface=Interface()
        return Config(interface)
    def write(self,*args): raise AssertionError('No bulk commands allowed')
    def read(self,*args): raise AssertionError('No bulk reads allowed')
    def reset(self): raise AssertionError('No reset allowed')


class Interface(list):
    bInterfaceClass=255;bInterfaceSubClass=66;bInterfaceProtocol=3
    def __init__(self): super().__init__([SimpleNamespace(bEndpointAddress=a,bmAttributes=2,wMaxPacketSize=512) for a in (1,129)])


class Config:
    bConfigurationValue=1
    def __init__(self, interface): self.interface=interface
    def __getitem__(self, key): assert key==(0,0);return self.interface


class Observation(unittest.TestCase):
    def test_descriptors_no_claim_or_transfer(self):
        util=Util();result=inspect_device(Device(),util)
        self.assertEqual(result['result'],'DESCRIPTORS_AVAILABLE');self.assertEqual(util.calls,['identity','dispose'])
        self.assertEqual(result['bulk_reads']+result['bulk_writes']+result['reset_commands'],0)
    def test_one_claim_and_release(self):
        util=Util();result=inspect_device(Device(),util,True)
        self.assertEqual(result['result'],'OPEN_AVAILABLE');self.assertEqual(util.calls,['identity','claim','release','dispose'])
    def test_exact_failed_phase_and_release(self):
        for failure,phase in (('identity','identity'),('configuration','active_configuration'),('claim','claim_interface')):
            util=Util(failure);result=inspect_device(Device(failure),util,True)
            self.assertEqual(result['result'],'REFUSED');self.assertEqual(result['failed_phase'],phase)
            self.assertEqual(util.calls[-1],'dispose');self.assertNotIn('release',util.calls)


if __name__=='__main__': unittest.main()
