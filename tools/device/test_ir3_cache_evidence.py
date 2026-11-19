"""Exact restore requires fresh real GPU bytes, stats and same library proof."""
import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from ir3_cache_evidence import verify,expected
from native_proof import validate,NATIVE_TESTS,digest


class IR3Evidence(unittest.TestCase):
    token='0123456789abcdef';fixture={'library_sha256':'a'*64}
    def fixture_log(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,'SWEET IR3 CACHE PROBE begin phases=3 steps=13,29,13 original_pages=1 fresh_pipelines=1',
              'SWEET IR3 CACHE PROBE input library_sha256='+self.fixture['library_sha256']]
        for phase,(step,total) in enumerate([(13,13),(29,42),(13,55)]):
            rows.append(f'SWEET IR3 CACHE PROBE phase={phase} step={step} total={total} sequence={phase+1} operations=1 checked_bytes=320 mismatches=0 hash={expected(phase)} valid=1')
            rows.append(f'SWEET IR3 CACHE PROBE stats phase={phase} hits=1 stores=0 rejected=0 bytes_read=128 exact_variant=1')
        rows+=['SWEET IR3 CACHE PROBE END result=0 phases=3 checked_bytes=960 final_allocated=0 cold_and_restore=1 original_pages=1','SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)
    def test_actual_raw_and_cache_restore(self):self.assertEqual(verify(self.fixture_log(),self.token,self.fixture)['samples'][0]['checked_bytes'],960)
    def test_false_hit_bytes_fence_guard_rejected(self):
        text=self.fixture_log()
        for bad in [text.replace(expected(2),'0'*16),text.replace('phase=2 hits=1','phase=2 hits=0'),
                    text.replace('phase=2 hits=1 stores=0 rejected=0 bytes_read=128','phase=2 hits=1 stores=0 rejected=0 bytes_read=0'),
                    text.replace('sequence=3','sequence=2'),text.replace('final_allocated=0','final_allocated=16384')]:
            with self.assertRaises(AssertionError):verify(bad,self.token,self.fixture)
    def test_fifteen_group_proof_cannot_admit_IR3(self):
        r={'native_IR3_variant_cache':True,'gles_library_sha256':'1'*64,'library_sha256':'2'*64,'source_files':{'metal':'3'*64}}
        p=dict(revision=1,version=730,native_run_token='4'*16,native_run_process='backboardd',kernel_sha256='5'*64,
               gles_library_sha256='1'*64,metal_library_sha256='2'*64,metal_source_files_sha256=digest(r['source_files']),
               native_run_image_sha256='6'*64,native_run_log_sha256='7'*64,
               native_tests=list(NATIVE_TESTS)+['AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half','Clear_cache'])
        with self.assertRaises(AssertionError):validate(p,r)
        p['native_tests'].append('IR3_cache');self.assertEqual(validate(p,r),p)


if __name__=='__main__':unittest.main()
