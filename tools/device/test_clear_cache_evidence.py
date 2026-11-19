"""A cached shader is useful only when all original attachment bytes agree."""
import unittest
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from clear_cache_evidence import verify,expected
from native_proof import NATIVE_TESTS,validate,digest


class ClearCacheEvidenceTests(unittest.TestCase):
    token='0123456789abcdef'
    fixture={'library_sha256':'a'*64}
    def fixture_log(self):
        rows=['SWEET LOOP LIVE proto=1 token='+self.token,
              'SWEET CLEAR CACHE begin phases=2 targets=8 width=17 height=9 pitch=128 offset=64 original_pages=1',
              'SWEET CLEAR CACHE input library_sha256='+self.fixture['library_sha256']]
        for phase in range(2):
            for index in range(8):rows.append(f'SWEET METAL CLEAR shader format={70 if index&1 else 80} slot=0 reused={int(phase>0 or index>0)} cached=1 result=0 original_shader=1 original_clear=1')
            for index in range(8):rows.append(f'SWEET CLEAR CACHE buffer phase={phase} index={index} hash={expected(index,phase)} original_pages=1')
            rows.append(f'SWEET CLEAR CACHE phase={phase} sequence={phase+10} operations=1 checked_bytes=131072 mismatches=0 status=4 error=0 valid=1')
        rows+=['SWEET CLEAR CACHE END result=0 phases=2 targets=8 checked_bytes=262144 final_allocated=0 original_clear=1 GPU_only=1','SWEET LOOP ACK FASTBOOT token='+self.token]
        return '\n'.join(rows)
    def test_original_bytes_and_cached_reuse(self):
        report=verify(self.fixture_log(),self.token,self.fixture)
        self.assertEqual(report['samples'][0]['reused_Clear_shaders'],15)
        self.assertEqual(report['samples'][0]['checked_bytes'],262144)
    def test_raw_guard_lifetime_reuse_and_fence_failures_refused(self):
        text=self.fixture_log()
        for value in (text.replace(expected(3,1),'0'*16),text.replace('phase=1 sequence=11','phase=1 sequence=10'),
                      text.replace('reused=1 cached=1','reused=0 cached=1',1),text.replace('final_allocated=0','final_allocated=16384'),
                      text.replace('original_pages=1','original_pages=0'),text.replace('phases=2 targets=8 checked_bytes=262144','phases=1 targets=8 checked_bytes=131072')):
            with self.subTest(value=value[:80]),self.assertRaises(AssertionError):verify(value,self.token,self.fixture)
    def test_old_fourteen_groups_cannot_admit_new_cache(self):
        resources=dict(native_Clear_shader_cache=True,gles_library_sha256='1'*64,library_sha256='2'*64,source_files={'metal':'3'*64})
        proof=dict(revision=1,version=730,native_run_token='4'*16,native_run_process='backboardd',
                   kernel_sha256='5'*64,gles_library_sha256='1'*64,metal_library_sha256='2'*64,
                   metal_source_files_sha256=digest(resources['source_files']),native_run_image_sha256='6'*64,native_run_log_sha256='7'*64,
                   native_tests=list(NATIVE_TESTS)+['AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half'])
        with self.assertRaises(AssertionError):validate(proof,resources)
        proof['native_tests'].append('Clear_cache');self.assertEqual(validate(proof,resources),proof)


if __name__=='__main__':unittest.main()
