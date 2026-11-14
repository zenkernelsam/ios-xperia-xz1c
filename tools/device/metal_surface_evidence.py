"""Verify native IOSurface render/sample bytes, logical origins, retention and fences."""
import re
from air_render_evidence import fnv
def expected(phase):
    raw=bytearray([0xa5]*1152);output=bytearray([0xe7]*1344)
    for y in range(9):
        for x in range(17):
            solid=phase>0 and 3<=x<14 and 2<=y<7
            pixel=bytes((191,128,64,255) if solid else (0,0,255,255))
            at=y*128+x*4;raw[at:at+4]=pixel
            if phase==2:output[128+at:132+at]=pixel
    return fnv(raw),fnv(output)
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET METAL SURFACE END result=[1-9]',scope),'IOSurface failure before return ACK'
    starts=list(re.finditer(r'SWEET METAL SURFACE begin cases=2 phases=3 width=17 height=9 pitch=128 original_IOSurface=1 caller_release=1',scope));assert starts
    records=[];incomplete=0
    for index,start in enumerate(starts):
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)]
        end=re.search(r'SWEET METAL SURFACE END result=(\d+) cases=(\d+) phases=6 checked_bytes=14976 row_padding=6480 final_allocated=(\d+) original_surface_GPU=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','2','0'),'IOSurface GPU/final lifetime failure';block=block[:end.start()]
        origins=list(re.finditer(r'SWEET METAL SURFACE backing offset=(64|128) id=(\d+) handle=(\d+) gpu=(0x[0-9a-f]+) cpu=(0x[0-9a-f]+) bytes=1152 original_origin=1 valid=1',block))
        retained=re.findall(r'SWEET METAL SURFACE retained offset=(64|128) caller_unmapped=1 caller_CF_released=1 texture_holds_surface=1',block)
        phases=list(re.finditer(r'SWEET METAL SURFACE phase offset=(64|128) phase=([012]) sequence=(\d+) operations=1 status=4 error=0 mismatches=0 raw_hash=([0-9a-f]{16}) output_hash=([0-9a-f]{16}) original_pages=1 valid=1',block))
        if len(origins)<2 or len(retained)<2 or len(phases)<6:incomplete+=1;continue
        assert [int(row[1]) for row in origins]==[64,128] and retained==['64','128']
        for row in origins:
            offset,identity,handle,gpu,cpu=int(row[1]),int(row[2]),int(row[3]),int(row[4],16),int(row[5],16)
            assert identity>0 and 0<handle<=4096 and 0x40000000<=gpu<0x80000000-1152 and cpu>0
            assert gpu&16383==offset and cpu&16383==offset
        assert [(int(row[1]),int(row[2])) for row in phases]==[(o,p) for o in (64,128) for p in range(3)]
        sequences=[int(row[3]) for row in phases];assert sequences==sorted(set(sequences)) and sequences[0]>0
        for row in phases:assert (row[4],row[5])==expected(int(row[2])),'Raw surface/output/padding bytes differ'
        for origin,offset in zip(origins,(64,128)):
            retain=block.index('SWEET METAL SURFACE retained offset='+str(offset))
            zero=next(row for row in phases if int(row[1])==offset and row[2]=='0')
            one=next(row for row in phases if int(row[1])==offset and row[2]=='1')
            assert origin.end()<zero.start()<retain<one.start()
        records.append(dict(sequences=sequences,logical_offsets=[64,128],checked_bytes=14976,row_padding_bytes=6480,
            actual_surface_ids=[int(row[2]) for row in origins],actual_handles=[int(row[3]) for row in origins],final_allocated=0))
    assert records,'No complete native IOSurface GPU sample'
    return dict(actual_IOSurface_Metal_GPU_render_sample_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        caller_mapping_and_CF_released_before_draw_sample=True,all_logical_bytes_and_row_padding_checked=True,full_UI_still_pending=True)
