"""Reject non-native, early-present, corrupt pixel or missing retirement evidence."""
import struct,unittest
from presentation_evidence import BEGIN,END,ABI,expected,verify
class PresentationEvidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'a'*64}
    def sample(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,BEGIN,'SWEET PRESENT PROBE INPUT fixture_sha256='+self.fixture['library_sha256'],ABI]
        for phase in range(5):
            mode=phase if phase<4 else 0;sequence=phase+1;identity=f'{0x100000000+phase:016x}';surface=100+phase;handle=10+phase;gpu=0x40000000+phase*16384
            time=17.125 if phase==1 else 0.0625 if phase==2 else 0;bits=struct.pack('<d',time)[::-1].hex();raw,pixels,padding=expected(128)
            rows += [f'SWEET PRESENT PROBE PAGE phase={phase} drawable={identity} surface={surface} handle={handle} GPU={hex(gpu)} pitch=128 bytes=1152 framebuffer_only=1 usage=4 valid=1',
                     f'SWEET PRESENT PROBE CPU phase={phase} framebuffer_read_rejected=1',
                     f'SWEET PRESENT PROBE REQUEST phase={phase} mode={mode} path={"transaction_wait" if phase==4 else "command"} time_bits={bits}',
                     f'SWEET AIR RENDER texture handle={handle} width=17 height=9 pitch=128 format=80 offset=0 gpu={hex(gpu)} original_pages=1',
                     f'SWEET METAL MRT retired index=0 handle={handle} format=80 result=0 sequence={sequence} original_pages=1',
                     f'SWEET AIR RENDER draw width=17 height=9 primitive=3 first=0 count=3 instances=1 target_handle={handle} result=0 sequence={sequence} original_pages=1',
                     f'SWEET DRAWABLE PUBLISH surface={surface} handle={handle} sequence={sequence} result=0 original_surface=1']
            if phase<4:rows += [f'SWEET PRESENT BEGIN mode={mode} drawable={identity} surface={surface} handle={handle} sequence={sequence} time_bits={bits} original_object=1',
                               f'SWEET PRESENT END mode={mode} surface={surface} sequence={sequence} result=0 original_present=1 display_completion_unverified=1']
            else:rows += ['SWEET PRESENT PROBE DIRECT phase=4 original_wait_then_present=1']
            rows += [f'SWEET PRESENT PROBE PHASE index={phase} valid=1 sequence={sequence} operations=1 checked_pixels=153 guard_bytes=540 mismatches=0',
                     f'SWEET PRESENT PROBE HASH index={phase} raw={raw} pixels={pixels} padding={padding}',
                     f'SWEET PRESENT PROBE NATIVE phase={phase} presented_time_bits=0000000000000000 optical_unverified=1']
        return '\n'.join(rows+[END,'SWEET LOOP ACK FASTBOOT token='+self.token])+'\n'
    def test_original_pixels_and_unproven_display_are_distinct(self):
        report=verify(self.sample(),self.token,self.fixture);self.assertEqual(report['checked_pixels'],765)
        self.assertTrue(report['native_display_completion_optical_and_original_UI_not_proven']);self.assertEqual(len(report['records']),5)
        self.assertNotEqual(expected(128)[0],expected(192)[0]);self.assertEqual(expected(128)[1],expected(192)[1])
    def test_pixel_padding_page_ABI_or_retirement_corruption_rejected(self):
        text=self.sample();raw,pixels,padding=expected(128)
        for bad in (text.replace('checked_pixels=765','checked_pixels=764'),text.replace('final_allocated=0','final_allocated=1152'),
                    text.replace('framebuffer_only=1','framebuffer_only=0',1),text.replace('pitch=128 bytes=1152','pitch=192 bytes=1152',1),
                    text.replace('GPU=0x40000000','GPU=0x40000001'),text.replace('QuartzCore_uuid=9e40','QuartzCore_uuid=0e40'),
                    text.replace('pixels='+pixels,'pixels='+'0'*16,1),text.replace('padding='+padding,'padding='+'0'*16,1),
                    text.replace('raw='+raw,'raw='+'0'*16,1),text.replace('sequence=2 operations=1','sequence=1 operations=1',1),
                    text.replace('SWEET METAL MRT retired index=0 handle=10 format=80 result=0 sequence=1 original_pages=1\n',''),
                    text.replace('SWEET DRAWABLE PUBLISH surface=100 handle=10 sequence=1 result=0 original_surface=1\n',''),
                    text.replace('framebuffer_read_rejected=1','framebuffer_read_rejected=0',1),text.replace('original_object=1','original_object=0',1),
                    text.replace('SWEET PRESENT PROBE HASH index=0','SWEET PRESENT PROBE UNKNOWN index=0')):
            with self.subTest(changed=bad!=text),self.assertRaises(AssertionError):self.assertNotEqual(bad,text);verify(bad,self.token,self.fixture)
    def test_early_present_wrong_time_or_postACK_result_rejected(self):
        text=self.sample();lines=text.splitlines();present=next(i for i,l in enumerate(lines) if l.startswith('SWEET PRESENT BEGIN'))
        draw=next(i for i,l in enumerate(lines) if l.startswith('SWEET AIR RENDER draw'));lines[draw],lines[present]=lines[present],lines[draw]
        wrong='\n'.join(lines)+'\n'
        for bad in (wrong,text.replace('time_bits=3fb0000000000000','time_bits=3fc0000000000000'),text.replace('path=transaction_wait','path=command'),
                    text.replace(END+'\n','')+END+'\n',text.replace('original_wait_then_present=1','original_wait_then_present=0')):
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
if __name__=='__main__':unittest.main()
