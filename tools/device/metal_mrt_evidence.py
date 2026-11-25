"""Exact simultaneous attachments, independent blend/masks, XR bit preservation and fences."""
import re,struct
from functools import lru_cache
from air_render_evidence import fnv
from air_xr_evidence import code,tables
def f32(value):return struct.unpack('<f',struct.pack('<f',value))[0]
@lru_cache(None)
def xr_codes(format,phase):
    srgb=bool(format&1);r,g,b=code(.125,srgb),code(.25,srgb),code(.875,srgb)
    if phase>=2:
        r=code(f32(decode(r,srgb)+.25),srgb);b=code(f32(decode(b,srgb)+.0625),srgb)
    return r,g,b
def decode(value,srgb):
    if srgb:return f32((1 if value>=384 else -1)*tables()[0][abs(value-384)]/8192)
    return f32((value-384)/510)
def sample_pixel(format,rgba):
    r,g,b=xr_codes(format,2);srgb=bool(format&1)
    def unorm(code_):return int(f32(f32(max(0,min(1,decode(code_,srgb)))*255)+.5))
    return bytes((unorm(r),unorm(g),unorm(b),255) if rgba else (unorm(b),unorm(g),unorm(r),255))
def expected(format,phase):
    raw=[bytearray([0xa5 if i==0 else 0xb6]*16384) for i in range(4)];pixel=8 if format<554 else 4;pitch=192 if format<554 else 128
    for y in range(9):
        for x in range(17):
            active=phase>0 and 3<=x<14 and 2<=y<7
            r,g,b=xr_codes(format,phase) if active else (384,384,894)
            if format<554:
                words=[b<<6,g<<6,r<<6,894<<6]
                if phase:
                    for c in range(4):
                        if not active or c==3:words[c]|=(x*3+y*5+c*7)&63
                stored=struct.pack('<4H',*words)
            else:stored=struct.pack('<I',(r<<20)|(g<<10)|b|(0xc0000000 if not phase or active else 0))
            at=64+y*pitch+x*pixel;raw[0][at:at+pixel]=stored
            at=128+y*128+x*4;raw[1][at:at+4]=bytes((255 if active and phase>=2 else 0,191 if active and phase>=2 else 64 if active else 0,255,255))
            if phase==3:
                raw[2][at:at+4]=sample_pixel(format,False);raw[3][at:at+4]=sample_pixel(format,True)
    return [fnv(p) for p in raw]
def eight_expected(index):
    raw=bytearray([0xa0+index]*16384);r=int(index*.125*255+.5);pixel=bytes((r,64,128,255) if index&1 else (128,64,r,255))
    for y in range(9):
        for x in range(17):at=64+y*128+x*4;raw[at:at+4]=pixel
    return fnv(raw)
def order_expected(format):
    srgb=bool(format&1);r,g,b=384,384,894
    for red in (.25,.375,.5,.625):
        r=code(f32(f32(red*.5)+f32(decode(r,srgb)*.5)),srgb)
        g=code(f32(-.125+f32(decode(g,srgb)*.5)),srgb)
        b=code(f32(.5625+f32(decode(b,srgb)*.5)),srgb)
    raw=bytearray([0xa5]*16384);pixel=8 if format<554 else 4;pitch=192 if format<554 else 128
    for y in range(9):
        for x in range(17):
            active=3<=x<14 and 2<=y<7;rr,gg,bb=(r,g,b) if active else (384,384,894)
            if format<554:
                words=[bb<<6,gg<<6,rr<<6,894<<6]
                for c in range(4):
                    if not active or c==3:words[c]|=(x*3+y*5+c*7)&63
                stored=struct.pack('<4H',*words)
            else:stored=struct.pack('<I',(rr<<20)|(gg<<10)|bb|(0xc0000000 if active else 0))
            at=64+y*pitch+x*pixel;raw[at:at+pixel]=stored
    return fnv(raw)
def verify(text,token,fixture,require_xr_clear_elision=False,require_image_barrier=False):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET METAL MRT END result=[1-9]',scope),'MRT probe failure before ACK'
    starts=list(re.finditer(r'SWEET METAL MRT begin XR_formats=552,553,554,555 dual_targets=2 phases=4 eight_targets=8 original_pages=1',scope));assert starts
    records=[];incomplete=0
    for i,start in enumerate(starts):
        block=scope[start.end():starts[i+1].start() if i+1<len(starts) else len(scope)]
        end=re.search(r'SWEET METAL MRT END result=(\d+) XR_formats=(\d+) phases=16 eight_targets=8 ordered_formats=(\d+) checked_bytes=1245184 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','4','4','0');block=block[:end.start()]
        assert 'SWEET METAL MRT input library_sha256='+fixture['library_sha256'] in block
        phases=list(re.finditer(r'SWEET METAL MRT phase format=(55[2-5]) phase=([0-3]) sequence=(\d+) operations=1 status=4 error=0 checked_bytes=65536 mismatches=0 valid=1',block))
        buffers=list(re.finditer(r'SWEET METAL MRT buffer format=(55[2-5]) phase=([0-3]) index=([0-3]) hash=([0-9a-f]{16}) original_pages=1',block))
        eight=re.search(r'SWEET METAL MRT eight sequence=(\d+) operations=1 checked_bytes=131072 mismatches=0 valid=1',block)
        extra=list(re.finditer(r'SWEET METAL MRT eight_buffer index=([0-7]) hash=([0-9a-f]{16}) original_pages=1',block))
        ordered=list(re.finditer(r'SWEET METAL MRT order format=(55[2-5]) first=3 count=6 instances=2 primitives=4 sequence=(\d+) operations=1 checked_bytes=16384 mismatches=0 valid=1 hash=([0-9a-f]{16}) original_pages=1 HDR_negative=1 HDR_above_one=1',block))
        if len(phases)<16 or len(buffers)<64 or not eight or len(extra)<8 or len(ordered)<4:incomplete+=1;continue
        assert [(int(p[1]),int(p[2])) for p in phases]==[(f,p) for f in range(552,556) for p in range(4)]
        assert [(int(p[1]),int(p[2]),int(p[3])) for p in buffers]==[(f,p,b) for f in range(552,556) for p in range(4) for b in range(4)]
        for index,p in enumerate(phases):
            format,phase=int(p[1]),int(p[2]);assert [row[4] for row in buffers[index*4:index*4+4]]==expected(format,phase),'Original XR codes/blend/masks/readback/padding differ'
            assert all(p.end()<row.start() for row in buffers[index*4:index*4+4])
        assert [int(row[1]) for row in extra]==list(range(8)) and [row[2] for row in extra]==[eight_expected(n) for n in range(8)]
        assert [int(p[1]) for p in ordered]==list(range(552,556)) and [p[3] for p in ordered]==[order_expected(f) for f in range(552,556)],'Original overlapping primitive/instance/HDR/guard bytes differ'
        assert len(re.findall(r'SWEET METAL MRT ordered_draw image_mask=1 primitives=2 instances=2 fp32=1 per_primitive_XR_round=1 image_barriers=1',block))==4,'Missing ordered destination updates for actual overlapping draws'
        if require_image_barrier:
            revision=int(require_image_barrier)
            assert revision in (1,2)
            barriers=re.findall(r'SWEET XR BARRIER revision='+str(revision)+r' primitives=(\d+) instances=(\d+) cached=(\d+) fallback=(\d+) '+('shader_write_wait' if revision==1 else 'timestamp_wait')+r'=1 UCHE_invalidate=1 final_fence_required=1',block)
            assert len(barriers)==12,'Original optimized XR barrier records missing'
            assert sum((int(n),int(inst),int(cached),int(fallback))==(2,2,4,0) for n,inst,cached,fallback in barriers)==4
            assert all(int(cached)==int(n)*int(inst) and int(fallback)==0 for n,inst,cached,fallback in barriers),'Actual optimized XR path fell back or skipped a primitive barrier'
        assert re.search(r'SWEET METAL MRT caps targets=8 fp32_rt=1 fp32_blend=0 fp32_image=1 independent_enable=1 independent_func=1 float_blend=0 fbfetch=0 no_attachment=1 fragment_images=\d+ image_units=\d+',block),'Actual A5xx capability record missing'
        seq=[int(p[3]) for p in phases]+[int(eight[1])]+[int(p[2]) for p in ordered];assert seq==sorted(set(seq)) and seq[0]>0
        # Each complete original target gets an independently retired submission.
        retired=list(re.finditer(r'SWEET METAL MRT retired index=([0-7]) handle=(\d+) format=(\d+) result=0 sequence=(\d+) original_pages=1',block))
        cursor=0
        for phase in phases+[eight]+ordered:
            group=[row for row in retired if cursor<row.start()<phase.start()];expected_indices=list(range(8)) if phase==eight else [0] if phase in ordered else [0,1]
            assert [int(row[1]) for row in group]==expected_indices,'Missing current-pass attachment retirement'
            assert all(0<int(row[2])<=4096 and int(row[4])>0 for row in group)
            assert max(int(row[4]) for row in group)==int(phase[1] if phase==eight else phase[2] if phase in ordered else phase[3]);cursor=phase.end()
        writebacks=re.findall(r'SWEET METAL MRT writeback format=(55[2-5]) handle=(\d+) mask=(10|15) result=0 GPU_only=1 original_code_snapshot=1',block)
        assert [int(p[0]) for p in writebacks]==[f for f in range(552,556) for _ in range(3)] and [int(p[2]) for p in writebacks]==[15,15,10]*4
        assert [int(f) for f in re.findall(r'SWEET METAL MRT writeback format=(55[2-5]) handle=\d+ mask=14 result=0 GPU_only=1 original_code_snapshot=1',block)]==list(range(552,556))
        if require_xr_clear_elision:
            clears=list(re.finditer(r'SWEET METAL MRT clear_target index=0 format=(55[2-5]) handle=(\d+) width=17 height=9 result=0 decode_skipped=1 original_code_snapshot=1 original_clear=1',block))
            assert [int(p[1]) for p in clears]==list(range(552,556)),'Missing current XR Clear optimization evidence'
            previous_end=0
            for n,clear in enumerate(clears):
                phase0=phases[n*4]
                assert previous_end<clear.start()<phase0.start() and 0<int(clear[2])<=4096
                prefix=block[previous_end:phase0.start()]
                assert 'SWEET METAL MRT decode format='+clear[1]+' ' not in prefix,'Clear still decoded discarded old colour'
                for load_phase in (1,2):
                    load_region=block[phases[n*4+load_phase-1].end():phases[n*4+load_phase].start()]
                    assert 'SWEET METAL MRT decode format='+clear[1]+' ' in load_region,'Original Load decoding is absent'
                previous_end=phases[n*4+3].end()
        records.append(dict(sequences=seq,XR_formats=[552,553,554,555],dual_passes=16,eight_targets=8,ordered_formats=4,checked_bytes=1245184,
            overlapping_primitives_and_instance_ID_and_nonzero_vertex_start_and_HDR_verified=True,
            all_four_original_buffers_and_padding_checked=True,unchanged_XR_lowbits_and_masked_channels_preserved=True,
            per_attachment_retirement_verified=True,XR_clear_decode_elision_verified=require_xr_clear_elision,
            original_Load_decode_preserved=require_xr_clear_elision,optimized_A5xx_image_barrier_verified=require_image_barrier,final_allocated=0))
    assert records,'No complete real simultaneous MRT/XR blend/guards sample'
    return dict(actual_native_MRT_XR_blend_channel_masks_and_eight_targets_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        full_original_UI_and_general_blend_factors_still_pending=True)
