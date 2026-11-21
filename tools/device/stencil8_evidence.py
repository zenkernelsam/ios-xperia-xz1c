"""Byte recipe and source binding for original separate Stencil8/Depth32 pages."""
import re
import struct

PHASES=99
BEGIN='SWEET STENCIL8 BEGIN phases=99 width=17 height=9 color_bytes=1280 depth_bytes=1280 stencil_bytes=768 original_pages=1'
PHASE=re.compile(r'SWEET STENCIL8 PHASE index=(\d+) valid=([01]) sequence=(\d+) operations=(\d+) mismatches=(\d+) read_mismatches=(\d+)')
HASH=re.compile(r'SWEET STENCIL8 HASH index=(\d+) color=([0-9a-f]{16}) depth=([0-9a-f]{16}) stencil=([0-9a-f]{16}) original_pages=([01])')
END=re.compile(r'SWEET STENCIL8 END result=(\d+) phases=(\d+) checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=(\d+) GPU_only=1')


def fnv(data):
    value=14695981039346656037
    for b in data:value=((value^b)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'


def operation(op,original,reference):
    return (original,0,reference,min(original+1,255),max(original-1,0),original^255,(original+1)&255,(original-1)&255)[op]


def compare(which,a,b):return (False,a<b,a==b,a<=b,a>b,a!=b,a>=b,True)[which]


def expected(phase):
    assert 0<=phase<99
    color=bytearray([0xa5]*1280);depth=bytearray([0xb6]*1280);stencil=bytearray([0xc7]*768)
    old=next_value=3;accepted=False
    if phase<48:
        i=phase%24;accepted=compare(i//3,2+i%3,3)
    elif phase<96:
        i=(phase-48)%24;op=i%8;back=(phase-48)//24
        old=(0,1,127,255)[(op+back)%4];mask=(0,255,15,255,255,240,255,255)[op]
        next_value=(operation(op,old,42)&mask)|(old&(~mask&255));accepted=i//8==2
    else:old=127 if phase==96 else 171;next_value=17;accepted=True
    for y in range(9):
        for x in range(17):
            box=3<=x<14 and 2<=y<7;at=64+y*128+x*4
            color[at:at+4]=bytes((191,128,64,255) if box and accepted else (255,0,255,255))
            depth[at:at+4]=struct.pack('<f',0.875 if phase==98 else 0.5)
            stencil[64+y*64+x]=171 if phase>=97 else next_value if box else old
    return fnv(color),fnv(depth),fnv(stencil)


def verify(text,token,fixture,tiled_revision=0):
    assert tiled_revision in (0,2,3,4)
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)];assert scope.count(BEGIN)==scope.count('SWEET STENCIL8 BEGIN ')==1
    block=scope[scope.index(BEGIN):];ends=list(END.finditer(block));assert len(ends)==1
    end=ends[0];assert end.groups()==('0','99','0'),'Original stencil operation or retirement failed'
    block=block[:end.start()];assert 'SWEET STENCIL8 INPUT fixture_sha256='+fixture['library_sha256'] in block
    phases=list(PHASE.finditer(block));hashes=list(HASH.finditer(block))
    assert len(phases)==len(hashes)==99 and block.count('SWEET STENCIL8 PHASE ')==block.count('SWEET STENCIL8 HASH ')==99
    assert [int(p[1]) for p in phases]==[int(h[1]) for h in hashes]==list(range(99))
    sequences=[];records=[]
    for index,(p,h) in enumerate(zip(phases,hashes)):
        assert p[2]=='1' and p[4]== '1' and p[5]==p[6]=='0' and int(p[3])>0 and h[5]=='1'
        assert (h[2],h[3],h[4])==expected(index),f'Original stencil/depth/color bytes differ at phase {index}'
        assert p.start()<h.start() and (index==98 or h.end()<phases[index+1].start())
        sequences.append(int(p[3]));records.append(dict(phase=index,sequence=int(p[3]),color_hash=h[2],depth_hash=h[3],stencil_hash=h[4]))
    assert sequences==sorted(set(sequences))
    imported=list(re.finditer(r'SWEET METAL STENCIL import handle=(\d+) width=17 height=9 pitch=64 offset=64 GPU=(0x[0-9a-f]+) original_pages=1 separate_z32s8=1',block))
    cleared=list(re.finditer(r'SWEET METAL STENCIL clear handle=(\d+) width=17 height=9 shader_draw=1 result=0 original_pages=1',block))
    retired=list(re.finditer(r'SWEET METAL STENCIL retired handle=(\d+) result=0 sequence=(\d+) original_pages=1',block))
    depths=list(re.finditer(r'SWEET METAL DEPTH import handle=(\d+) width=17 height=9 pitch=128 offset=64 GPU=(0x[0-9a-f]+) original_pages=1',block))
    assert len(imported)==len(retired)==99 and len(cleared)==98 and len(depths)==98
    assert len(set(m.groups() for m in imported))==len(set(m.groups() for m in depths))==1
    handle=imported[0][1];depth_handle=depths[0][1]
    assert int(handle)>0 and int(depth_handle)>0 and handle!=depth_handle
    assert int(imported[0][2],16)%16384==int(depths[0][2],16)%16384==0
    assert all(m[1]==handle for m in cleared+retired) and all(0<int(m[2])<=s for m,s in zip(retired,sequences))
    assert block.count('SWEET METAL STENCIL import ')==99 and block.count('SWEET METAL STENCIL clear ')==98 and block.count('SWEET METAL STENCIL retired ')==99
    tiled=None
    if tiled_revision:
        from depth_tiled_evidence import plane,verify_groups
        groups=[]
        for phase in range(99):
            group=[]
            if phase!=96:group.append(plane(depth_handle,depths[0][2],252,1,128,2 if phase<96 or phase==98 else 1,publish=phase!=97))
            group.append(plane(handle,imported[0][2],253,1,64,1 if phase==98 else 2,publish=phase!=98))
            groups.append(group)
        tiled=verify_groups(block,groups,tiled_revision)
    result=dict(complete_samples=1,incomplete_samples=0,phases=99,checked_bytes=329472,guard_bytes=193149,read_bytes=118800,
                records=records,original_stencil_handle=int(handle),original_depth_handle=int(depth_handle),original_GPU_offset=64,
                original_stencil_GPU_address=imported[0][2],original_depth_GPU_address=depths[0][2],
                original_separate_planes_front_back_compares_all_operations_masks_Load_Clear_and_padding_verified=True,
                final_allocated=0,MSAA_XR_and_full_UI_not_inferred=True)
    if tiled is not None:result['tiled_work_planes']=tiled
    return result
