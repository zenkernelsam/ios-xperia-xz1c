"""Recheck complete native GPU and original PNG references from immutable raw."""
import json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from png_encoder_proof import validate
from trial_evidence import verify

def inspect(directory,resources,kernel_sha256,encoder_source_sha256,version=None):
    row=json.loads((Path(directory)/'results.json').read_bytes())[-1]
    actual_version=row['artifact']['version']
    expected_version=actual_version if version is None else version
    assert type(expected_version) is int and expected_version>=729 and actual_version==expected_version, 'PNG reference kernel version differs'
    actual=verify(directory,'gpu',kernel_sha256,expected_version)
    observer=row['artifact']['exception_observer']
    assert observer['target_process']=='backboardd' and observer.get('cif10_reference_shared_encoder')
    assert observer['png_imageio_encoder_source_sha256']==encoder_source_sha256
    png=actual['exceptions']['CIF10_PNG_reference']
    assert png['encoder_source_sha256']==encoder_source_sha256 and png['filter_policies_match_original']['up']
    assert len(png['cases'])==6 and all(c['filters']['up']['pixels_equal'] and c['filters']['up']['ICC_equal'] and c['filters']['up']['metadata_equal'] for c in png['cases'])
    for key in ('Metal_resources','Metal_queue','AIR_compute','AIR_render','AIR_constants','AIR_XR','Metal_IOSurface','Metal_mipmaps','AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half'):
        assert key in actual['exceptions']
    if resources.get('native_Clear_shader_cache'):
        cache=actual['exceptions']['Clear_cache'];assert cache['complete_samples']==1 and cache['incomplete_samples']==0
    if resources.get('native_IR3_variant_cache'):
        cache=actual['exceptions']['IR3_cache'];assert cache['complete_samples']==1 and cache['incomplete_samples']==0
    proof=dict(revision=1,kernel_sha256=kernel_sha256,GLES_sha256=actual['ram_library_binding']['library_sha256'],
        Metal_sha256=row['artifact']['metal_resources']['library_sha256'],encoder_source_sha256=encoder_source_sha256,
        reference_token=actual['token'],reference_image_sha256=actual['image_sha256'],reference_raw_sha256=actual['log_sha256'],
        reference_cases=6,filter=32,pixels_ICC_metadata_equal=True)
    return validate(proof,encoder_source_sha256,resources['gles_library_sha256'],resources['library_sha256'],kernel_sha256)

if __name__=='__main__':
    import argparse,hashlib
    p=argparse.ArgumentParser();p.add_argument('--directory',required=True);p.add_argument('--ram',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    ram=json.loads(Path(a.ram,'manifest.json').read_bytes());sha=hashlib.sha256((ROOT/'gpu/compat/PNGImageIOEncoder.h').read_bytes()).hexdigest()
    kernel=json.loads(Path(a.directory,'results.json').read_bytes())[-1]['artifact']['kernel_sha256']
    r=inspect(a.directory,ram['metal_resources'],kernel,sha);out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists()
    out.write_bytes((json.dumps(r,indent=2)+'\n').encode('utf-8'));print(json.dumps(dict(reference_token=r['reference_token'],encoder_source_sha256=sha,actual_reference_verified=True)))
