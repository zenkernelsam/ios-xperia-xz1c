"""A new encoder cannot reuse a different source, library or failed reference."""
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from png_encoder_proof import validate,header
p=dict(revision=1,kernel_sha256='1'*64,GLES_sha256='2'*64,Metal_sha256='3'*64,encoder_source_sha256='4'*64,
       reference_token='5'*16,reference_image_sha256='6'*64,reference_raw_sha256='7'*64,reference_cases=6,filter=32,pixels_ICC_metadata_equal=True)
assert validate(p,'4'*64,'2'*64,'3'*64,'1'*64)==p and b'SWEET_PNG_REFERENCE_TOKEN' in header(p)
for key,value in (('encoder_source_sha256','8'*64),('GLES_sha256','8'*64),('Metal_sha256','8'*64),('filter',8),('reference_cases',5),('pixels_ICC_metadata_equal',False),('reference_token','oops')):
    bad={**p,key:value}
    try:validate(bad,'4'*64,'2'*64,'3'*64,'1'*64)
    except AssertionError:continue
    raise AssertionError('Mismatched encoder prerequisite accepted')
print('PNG encoder proof PASS: exact source/kernel/GLES/Metal, full reference/filter/semantic and token rejection')
