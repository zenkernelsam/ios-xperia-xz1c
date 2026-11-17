"""Exact native PNG pixel/profile/metadata prerequisite for a shared encoder."""
import hashlib,json,re

FIELDS=('revision','kernel_sha256','GLES_sha256','Metal_sha256','encoder_source_sha256','reference_token','reference_image_sha256','reference_raw_sha256','reference_cases','filter','pixels_ICC_metadata_equal')
def validate(proof,encoder_source_sha256,GLES_sha256,Metal_sha256,kernel_sha256=None):
    assert set(proof)==set(FIELDS) and proof['revision']==1
    for key in ('kernel_sha256','GLES_sha256','Metal_sha256','encoder_source_sha256','reference_image_sha256','reference_raw_sha256'):
        assert re.fullmatch('[0-9a-f]{64}',proof[key]),key
    assert re.fullmatch('[0-9a-f]{16}',proof['reference_token']) and proof['reference_cases']==6 and proof['filter']==32 and proof['pixels_ICC_metadata_equal'] is True
    assert proof['encoder_source_sha256']==encoder_source_sha256 and proof['GLES_sha256']==GLES_sha256 and proof['Metal_sha256']==Metal_sha256
    if kernel_sha256:assert proof['kernel_sha256']==kernel_sha256
    return dict(proof)
def digest(proof):return hashlib.sha256(json.dumps(proof,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def header(proof):
    return (f'#define SWEET_PNG_REFERENCE_TOKEN "{proof["reference_token"]}"\n'
            f'#define SWEET_PNG_IMAGEIO_SOURCE_SHA "{proof["encoder_source_sha256"]}"\n'
            f'#define SWEET_PNG_REFERENCE_RAW_SHA "{proof["reference_raw_sha256"]}"\n'
            f'#define SWEET_PNG_ENCODER_PROOF_SHA "{digest(proof)}"\n').encode()
