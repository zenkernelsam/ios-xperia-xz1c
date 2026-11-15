import struct
import unittest
from air_render_evidence import fnv
from static_half_evidence import PATTERNS,verify


class StaticHalfEvidenceTests(unittest.TestCase):
    token='0123456789abcdef'
    fixture={'library_sha256':'b'*64}

    def capture(self):
        lines=['SWEET LOOP LIVE proto=1 token='+self.token,
               'SWEET AIR STATIC HALF begin cases=12 paths=4 original_constants=1 original_pages=1 GPU_only=1',
               'SWEET AIR STATIC HALF input library_sha256='+self.fixture['library_sha256']]
        for n,h in enumerate(PATTERNS):
            w=PATTERNS[(n+1)%12];raw=bytearray([0xa5]*16384);struct.pack_into('<4I',raw,64,h,h,w,w)
            lines += [f'SWEET AIR STATIC HALF case={n} half={h:04x} word={w:04x} status=4 error=0 sequence={101+n} operations=1 checked_bytes=16384 mismatches=0 hash={fnv(raw)} frozen_after_reset=1 valid=1',
                      f'SWEET AIR STATIC HALF run case={n} offset=0 bytes=64 pattern=a5',
                      f'SWEET AIR STATIC HALF data case={n} offset=64 bytes=16 hex={raw[64:80].hex()}',
                      f'SWEET AIR STATIC HALF run case={n} offset=80 bytes=16304 pattern=a5']
        lines += ['SWEET AIR STATIC HALF END result=0 cases=12 paths=4 checked_bytes=196608 final_allocated=0 original_constants=1 GPU_only=1',
                  'SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(lines)

    def test_all_original_bytes(self):
        result=verify(self.capture(),self.token,self.fixture)
        self.assertEqual(result['samples'][0]['bytes_checked'],196608)

    def test_faults_refused(self):
        text=self.capture()
        cases=[text.replace('bytes=16304','bytes=16303',1),
               text.replace('offset=80','offset=81',1),
               text.replace('sequence=102','sequence=101',1),
               text.replace('half=7d01','half=7e01',1),
               text.replace('pattern=a5','pattern=a4',1),
               text.replace('frozen_after_reset=1 valid=1','frozen_after_reset=1 valid=0',1),
               text.replace('final_allocated=0','final_allocated=16384',1)]
        for case in cases:
            with self.subTest(case=case),self.assertRaises(AssertionError):verify(case,self.token,self.fixture)

    def test_changed_data_and_source_refused(self):
        text=self.capture()
        begin=text.index('hex=')+4;bad=text[:begin]+'01'+text[begin+2:]
        with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
        with self.assertRaises(AssertionError):verify(text,self.token,{'library_sha256':'c'*64})


if __name__=='__main__':unittest.main(verbosity=2)
