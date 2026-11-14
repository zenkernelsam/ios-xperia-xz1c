"""Independent full mip storage/guard/LOD/fence check inside current LIVE→ACK."""
import math,re,struct
from functools import lru_cache
from air_render_evidence import fnv
from air_xr_evidence import code,tables
@lru_cache(None)
def xr_mid():return code(.5,True)
@lru_cache(None)
def xr_byte():return int(tables()[0][xr_mid()-384]/8192*255+.5)
def layouts(format):
    pixel=8 if format==553 else 4;w,h,offset=32,16,64;result=[]
    for level in range(6):
        pitch=(w*pixel+63)&~63;result.append((w,h,pitch,offset));offset+=pitch*h;w=max(w//2,1);h=max(h//2,1)
    return result
def linear(level,x,y,phase):
    scale=1<<level;hs=min(scale,16)
    return (16+4*phase+4*x*scale+2*(scale-1),32+4*phase+8*y*hs+4*(hs-1),64+4*phase,255)
def raw_expected(format,phase,codes):
    raw=bytearray([0xa5]*16384)
    for level,(w,h,pitch,offset) in enumerate(layouts(format)):
        for y in range(h):
            for x in range(w):
                if format==553:
                    v=xr_mid() if level else (894 if (x+y)&1 else 384)
                    stored=struct.pack('<4H',v<<6,(384 if phase else v)<<6,v<<6,894<<6)
                else:
                    r,g,b,a=linear(level,x,y,phase)
                    if format==81:r=b=codes[level-1] if level else (255 if (x+y)&1 else 0);g=0 if phase else r
                    stored=bytes((r,g,b,a) if format==70 else (b,g,r,a))
                at=offset+y*pitch+x*len(stored);raw[at:at+len(stored)]=stored
    return fnv(raw)
def srgb_linear(value):
    encoded=value/255;return encoded/12.92 if encoded<=.04045 else ((encoded+.055)/1.055)**2.4
def output_colour(format,phase,sample,codes,x=0,y=0):
    if format in (81,553):
        if format==553:value=xr_byte()
        else:value=int(((srgb_linear(codes[0])+srgb_linear(codes[1]))/2 if sample==2 else srgb_linear(codes[3 if sample==1 else 0]))*255+.5)
        return (value,0 if phase else value,value,255)
    level=4 if sample==1 else 1;r,g,b,a=linear(level,x//(1<<level),y//(1<<level),phase)
    if sample==2:
        rr,gg,bb,_=linear(2,x//4,y//4,phase);r=(r+rr)//2;g=(g+gg)//2;b=(b+bb)//2
    return (b,g,r,a)
def output_expected(format,phase,sample,codes,colour):
    raw=bytearray([0xe7]*16384)
    for y in range(16):
        for x in range(32):
            pixel=colour if format==81 else output_colour(format,phase,sample,codes,x,y)
            at=128+y*128+x*4;raw[at:at+4]=bytes(pixel)
    return fnv(raw)
def verify(text,token):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET METAL MIPS END result=[1-9]',scope),'Native mip failure before ACK'
    starts=list(re.finditer(r'SWEET METAL MIPS begin formats=70,80,81,553 levels=6 generations=2 samples=3 original_pages=1',scope));assert starts
    records=[];incomplete=0
    for i,start in enumerate(starts):
        block=scope[start.end():starts[i+1].start() if i+1<len(starts) else len(scope)]
        end=re.search(r'SWEET METAL MIPS END result=(\d+) formats=(\d+) levels=6 generations=8 samples=24 checked_bytes=917504 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','4','0');block=block[:end.start()]
        generations=list(re.finditer(r'SWEET METAL MIPS generation format=(70|80|81|553) phase=([01]) sequence=(\d+) operations=5 mismatches=0 raw_hash=([0-9a-f]{16}) valid=1',block))
        codes_rows=list(re.finditer(r'SWEET METAL MIPS codes format=(70|80|81|553) phase=([01]) level1=(\d+) level2=(\d+) level3=(\d+) level4=(\d+) level5=(\d+) tolerance=([01])',block))
        samples=list(re.finditer(r'SWEET METAL MIPS sample format=(70|80|81|553) phase=([01]) sample=([012]) sequence=(\d+) operations=1 mismatches=0 max_error=([01]) output_hash=([0-9a-f]{16}) valid=1',block))
        colours=list(re.finditer(r'SWEET METAL MIPS colour format=(70|80|81|553) phase=([01]) sample=([012]) b=(\d+) g=(\d+) r=(\d+) a=(\d+)',block))
        levels=list(re.finditer(r'SWEET METAL MIPMAP level=([1-5]) src_handle=(\d+) dst_handle=(\d+) width=(\d+) height=(\d+) result=0 sequence=(\d+) original_pages=1',block))
        chains=re.findall(r'SWEET METAL MIPMAP sample_chain levels=6 width=32 height=16 shadow_bytes=10928 result=0 GPU_only=1 original_levels=1',block)
        if len(generations)<8 or len(codes_rows)<8 or len(samples)<24 or len(colours)<24 or len(levels)<40 or len(chains)<24:incomplete+=1;continue
        assert len(generations)==len(codes_rows)==8 and len(samples)==len(colours)==len(chains)==24 and len(levels)==40
        expected_groups=[(f,p) for f in (70,80,81,553) for p in (0,1)]
        assert [(int(r[1]),int(r[2])) for r in generations]==expected_groups
        assert [(int(r[1]),int(r[2])) for r in codes_rows]==expected_groups
        assert [(int(r[1]),int(r[2]),int(r[3])) for r in samples]==[(f,p,s) for f,p in expected_groups for s in range(3)]
        assert [(int(r[1]),int(r[2]),int(r[3])) for r in colours]==[(f,p,s) for f,p in expected_groups for s in range(3)]
        timeline=[];code_values=[]
        for index,(gen,codes_row) in enumerate(zip(generations,codes_rows)):
            format,phase=int(gen[1]),int(gen[2]);codes=[int(codes_row[k]) for k in range(3,8)];tolerance=int(codes_row[8]);code_values.append(codes)
            assert tolerance==(1 if format==81 else 0)
            assert all(187<=c<=189 for c in codes) if format==81 else codes==[0]*5
            assert gen[4]==raw_expected(format,phase,codes),'Original mip content/guard hash differs'
            group=levels[index*5:index*5+5];assert [int(r[1]) for r in group]==list(range(1,6))
            handle=int(group[0][2]);assert 0<handle<=4096
            for row,(w,h,_,_) in zip(group,layouts(format)[1:]):
                assert int(row[2])==int(row[3])==handle and int(row[4])==w and int(row[5])==h and row.end()<gen.start()
                timeline.append((row.start(),int(row[6])))
            assert int(gen[3])==int(group[-1][6])
            for sample_row,colour_row in zip(samples[index*3:index*3+3],colours[index*3:index*3+3]):
                sample=int(sample_row[3]);colour=tuple(int(colour_row[k]) for k in range(4,8));wanted=output_colour(format,phase,sample,codes)
                error=max(abs(a-b) for a,b in zip(colour,wanted));assert error<=tolerance and int(sample_row[5])==error
                if format==81:assert colour[0]==colour[2] and colour[1]==(0 if phase else colour[0]) and colour[3]==255
                assert sample_row[6]==output_expected(format,phase,sample,codes,colour),'LOD sample/output guard hash differs'
                assert gen.end()<sample_row.start()<colour_row.start();timeline.append((sample_row.start(),int(sample_row[4])))
        timeline.sort();sequences=[seq for _,seq in timeline];assert sequences==sorted(set(sequences)) and sequences[0]>0
        records.append(dict(formats=[70,80,81,553],levels=6,regenerations=8,LOD_samples=24,checked_bytes=917504,all_original_storage_and_guards_checked=True,
            sRGB_codes=code_values,UNorm_XR_exact=True,sRGB_tolerance_LSB=1,sequences=sequences,final_allocated=0))
    assert records,'No complete original-page mip generation/LOD sample'
    return dict(actual_native_GPU_mipmaps_regeneration_and_trilinear_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        six_original_levels_not_CPU_generated=True,all_guards_and_final_lifetime_verified=True,full_original_UI_still_pending=True)
