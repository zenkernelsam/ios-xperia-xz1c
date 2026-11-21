"""Exact four-property ADT geometry and numeric flags for a native XZ1C trial."""
import hashlib
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools/build'))
from make_render_adt import convert
from numeric_boot_args import PARSER_SHA
PROFILE='xz1c-native-2x'
NATIVE_ONES=('sweetfb_native','sweetbattery','ufsrw','rootrw','cs_enforcement_disable',
             'amfi_get_out_of_my_way','sweetfb_noconsole','lilac_usbdbg','sweetrmi4','sweetloopdrain','ssca')

def transform(reference,profile):
    assert profile==PROFILE
    data,proof=convert(reference,profile)
    binding=dict(revision=1,profile=PROFILE,canvas_pixels=[720,1280],UIKit_points=[360,640],scale=2,
                 source_reference_sha256=hashlib.sha256(reference).hexdigest(),target_reference_sha256=hashlib.sha256(data).hexdigest(),
                 converter_sha256=hashlib.sha256((ROOT/'tools/build/make_render_adt.py').read_bytes()).hexdigest(),
                 parser_sha256=PARSER_SHA,required_boot_arguments={'sweetfb_native':1,'sf2x':0},
                 four_original_CFData_properties_only=proof['other_properties_byte_exact'])
    assert binding['four_original_CFData_properties_only']
    return data,binding

def arguments(words):
    result=[w for w in words if w.split('=',1)[0] not in ('sf2x','sweetfb_native')]
    result+=['sf2x=0','sweetfb_native=1'];return result

def compact(words,names,limit=607):
    for name in NATIVE_ONES:
        if len(' '.join(words).encode())<=limit:break
        if name+'=1' in words:
            words=[name if w==name+'=1' else w for w in words];names.append(name)
    assert len(' '.join(words).encode())<=limit,'Native geometry command exceeds the fixed payload area'
    return words
