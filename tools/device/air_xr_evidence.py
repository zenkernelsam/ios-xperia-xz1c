"""Independently check raw XR code storage, decoded sampling, all padding and fences."""
import re,struct
from pathlib import Path
from air_render_evidence import fnv
ROOT=Path(__file__).resolve().parents[2]
def tables():
    header=(ROOT/'gpu/sweetgl/XRColourTables.h').read_text()
    def read(name):
        m=re.search(name+r'\[640\] = \{(.*?)\};',header,re.S);assert m
        result=[int(n) for n in re.findall(r'\d+',m[1])];assert len(result)==640;return result
    return read('sweet_xr_srgb_decode_steps'),read('sweet_xr_srgb_encode_first')
def code(value,srgb):
    if srgb:
        _,first=tables();index=round(abs(value)*8192);amount=max(i for i,v in enumerate(first) if v<=index)
    else:amount=round(abs(value)*510)
    return 384-min(amount,384) if value<0 else min(384+amount,1023)
def expected(format,phase):
    srgb=bool(format&1);pixel=8 if format<554 else 4;pitch=192 if format<554 else 128
    raw=bytearray([0xa5]*(128+pitch*9));output=bytearray([0xe7]*1344);decode,_=tables()
    for y in range(9):
        for x in range(17):
            solid=phase>0 and 3<=x<14 and 2<=y<7;colour=(.25,.5,.75) if solid else (1,0,0)
            r,g,b=[code(c,srgb) for c in colour]
            stored=struct.pack('<4H',b<<6,g<<6,r<<6,894<<6) if format<554 else struct.pack('<I',0xc0000000|(r<<20)|(g<<10)|b)
            offset=64+y*pitch+x*pixel;raw[offset:offset+pixel]=stored
            if phase==2:
                def channel(code):
                    value=decode[abs(code-384)]/8192 if srgb else (code-384)/510
                    if srgb and code<384:value=-value
                    return int(max(0,min(1,value))*255+.5)
                out=128+y*128+x*4;output[out:out+4]=bytes((channel(b),channel(g),channel(r),255))
    return fnv(raw),fnv(output),len(raw)
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    starts=list(re.finditer(r'SWEET AIR XR begin formats=552,553,554,555 standard_texture_render=1 raw_code_storage=1 GPU_sample_decode=1',scope));assert starts
    records=[];incomplete=0
    for index,start in enumerate(starts):
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)]
        end=re.search(r'SWEET AIR XR END result=(\d+) formats=(\d+) phases=12 checked_raw_output_bytes=34944 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','4','0'),'XR raw/sample/guard/lifetime failed';block=block[:end.start()]
        rows=list(re.finditer(r'SWEET AIR XR format=(55[2-5]) phase=([0-2]) status=4 error=0 sequence=(\d+) operations=1 raw_bytes=(1856|1280) output_bytes=1344 pixels=153 mismatches=0 raw_hash=([0-9a-f]{16}) output_hash=([0-9a-f]{16}) original_pages=1 valid=1',block))
        if len(rows)<12:incomplete+=1;continue
        assert [(int(row[1]),int(row[2])) for row in rows]==[(f,p) for f in range(552,556) for p in range(3)]
        sequence=[int(row[3]) for row in rows];assert sequence==sorted(set(sequence)) and sequence[0]>0
        for row in rows:
            raw,out,bytes_=expected(int(row[1]),int(row[2]));assert (row[5],row[6],int(row[4]))==(raw,out,bytes_)
        decoded=re.findall(r'SWEET AIR XR sample_decode format=(55[2-5]) handle=(\d+) width=17 height=9 result=0 GPU_only=1 original_source_pages=1 float_shadow=1',block)
        assert [int(row[0]) for row in decoded]==list(range(552,556)) and all(int(row[1])>0 for row in decoded)
        records.append(dict(sequences=sequence,formats=[552,553,554,555],raw_hashes=[row[5] for row in rows],output_hashes=[row[6] for row in rows],
            checked_raw_output_bytes=34944,pixels_per_pass=153,all_padding_prefix_suffix_checked=True,final_allocated=0))
    assert records,'No complete XR format hardware sample'
    return dict(actual_XR_4_formats_raw_GPU_render_decode_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        extended_linear_sRGB_original_bytes_and_GPU_float_shadow_verified=True,original_UI_and_unimplemented_XR_blend_still_pending=True)
