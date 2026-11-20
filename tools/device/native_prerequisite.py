"""Reverify actual complete background native logs before short original registration."""
import argparse,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from native_proof import NATIVE_TESTS,digest,validate
from trial_evidence import verify as verify_trial
def verify(directory,resources=None,kernel_sha=None,version=None):
    root=Path(directory);row=json.loads((root/'results.json').read_bytes())[-1];artifact=row['artifact']
    observer=artifact.get('exception_observer',{})
    assert observer.get('target_process')=='backboardd' and observer.get('background_full_suite') and not observer.get('registration_only'),'Prior must be an actual full native background stage'
    evidence=verify_trial(root,'gpu',kernel_sha,version)
    suite=evidence['exceptions'];assert suite['background_probe']['probe_suites']==1
    native_tests=list(NATIVE_TESTS)+(['AIR_narrow'] if artifact['metal_resources'].get('AIR_frontend',{}).get('local_narrow_conversion_adaptation') else [])
    if artifact['metal_resources'].get('AIR_frontend',{}).get('local_half_storage_and_rounding_adaptation'):native_tests.append('AIR_half')
    if artifact['metal_resources'].get('native_MRT_and_XR_blend'):native_tests.append('Metal_MRT')
    if artifact['metal_resources'].get('native_AIR_constant_samplers'):native_tests.append('AIR_sampler')
    if artifact['metal_resources'].get('native_program_binary_cache'):native_tests.append('Program_cache')
    if artifact['metal_resources'].get('native_static_half_bits'):native_tests.append('AIR_static_half')
    if artifact['metal_resources'].get('native_Clear_shader_cache'):native_tests.append('Clear_cache')
    if artifact['metal_resources'].get('native_IR3_variant_cache'):native_tests.append('IR3_cache')
    if artifact['metal_resources'].get('native_depth32_attachment'):native_tests.append('Metal_depth32')
    if artifact['metal_resources'].get('native_stencil8_attachment'):native_tests.append('Metal_stencil8')
    for name in native_tests:
        assert suite[name]['complete_samples']>=1 and suite[name]['incomplete_samples']==0,'Prior native probe incomplete: '+name
    native=artifact['metal_resources']
    proof=dict(revision=1,version=artifact['version'],kernel_sha256=artifact['kernel_sha256'],
        gles_library_sha256=native['gles_library_sha256'],metal_library_sha256=native['library_sha256'],metal_source_files_sha256=digest(native['source_files']),
        native_run_token=artifact['token'],native_run_image_sha256=artifact['sha256'],native_run_log_sha256=evidence['log_sha256'],native_run_process='backboardd',native_tests=native_tests)
    return validate(proof,resources)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    proof=verify(a.directory);out=Path(a.output).resolve();assert not out.exists();out.parent.mkdir(parents=True,exist_ok=True)
    out.write_bytes((json.dumps(proof,indent=2)+'\n').encode());print(json.dumps(proof))
