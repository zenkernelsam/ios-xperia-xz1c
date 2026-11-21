"""Independent byte recipe for real Depth32Float compare/load/clear frames."""
import re
import struct

PHASES=29
BEGIN='SWEET DEPTH32 BEGIN phases=29 width=17 height=9 bytes=1280 offset=64 pitch=128 original_pages=1'
PHASE=re.compile(r'SWEET DEPTH32 PHASE index=(\d+) valid=([01]) sequence=(\d+) operations=(\d+) mismatches=(\d+) read_mismatches=(\d+)')
HASH=re.compile(r'SWEET DEPTH32 HASH index=(\d+) color=([0-9a-f]{16}) depth=([0-9a-f]{16}) original_pages=([01])')
END=re.compile(r'SWEET DEPTH32 END result=(\d+) phases=(\d+) checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=(\d+) GPU_only=1')


def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'


def compare(which,a,b):
    return (False,a<b,a==b,a<=b,a>b,a!=b,a>=b,True)[which]


def expected(phase):
    assert 0<=phase<PHASES
    color=bytearray([0xa5]*1280);depth=bytearray([0xb6]*1280)
    passed=compare(phase//3,(phase%3+1)*0.25,0.5) if phase<24 else phase in (24,26)
    for y in range(9):
        for x in range(17):
            box=3<=x<14 and 2<=y<7;at=64+y*128+x*4
            color[at:at+4]=bytes((191,128,64,255) if passed and box else (255,0,255,255))
            value=0.5 if phase<24 else 0.875 if phase==28 else 0.25 if box else 0.75
            depth[at:at+4]=struct.pack('<f',value)
    return fnv(color),fnv(depth)


def verify(text,token,fixture,tiled_revision=0):
    assert tiled_revision in (0,2,3,4,5,6,7,8,9)
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)]
    assert scope.count(BEGIN)==1 and scope.count('SWEET DEPTH32 BEGIN ')==1,'One actual depth suite required'
    block=scope[scope.index(BEGIN):];end=list(END.finditer(block));assert len(end)==1,'Complete depth suite missing'
    end=end[0];assert end.groups()==('0','29','0'),'Actual depth/guard/lifetime failure'
    block=block[:end.start()]
    assert 'SWEET DEPTH32 INPUT fixture_sha256='+fixture['library_sha256'] in block
    phases=list(PHASE.finditer(block));hashes=list(HASH.finditer(block))
    assert len(phases)==len(hashes)==29 and block.count('SWEET DEPTH32 PHASE ')==block.count('SWEET DEPTH32 HASH ')==29
    assert [int(p[1]) for p in phases]==[int(h[1]) for h in hashes]==list(range(29))
    rows=[];sequences=[]
    for phase,(p,h) in enumerate(zip(phases,hashes)):
        assert p[2]=='1' and (p[4],p[5],p[6]) == ('1','0','0') and int(p[3])>0 and h[4]=='1'
        assert (h[2],h[3])==expected(phase),f'Original depth/color bytes differ at phase {phase}'
        assert p.start()<h.start() and (phase==28 or h.end()<phases[phase+1].start())
        sequences.append(int(p[3]));rows.append(dict(phase=phase,sequence=int(p[3]),color_hash=h[2],depth_hash=h[3]))
    assert sequences==sorted(set(sequences))
    imported=list(re.finditer(r'SWEET METAL DEPTH import handle=(\d+) width=17 height=9 pitch=128 offset=64 GPU=(0x[0-9a-f]+) original_pages=1',block))
    cleared=list(re.finditer(r'SWEET METAL DEPTH clear handle=(\d+) width=17 height=9 shader_draw=1 result=0 original_pages=1',block))
    retired=list(re.finditer(r'SWEET METAL DEPTH retired handle=(\d+) result=0 sequence=(\d+) original_pages=1',block))
    assert len(imported)==29 and len(cleared)==26 and len(retired)==29
    assert block.count('SWEET METAL DEPTH import ')==29 and block.count('SWEET METAL DEPTH clear ')==26 and block.count('SWEET METAL DEPTH retired ')==29
    assert len(set(m.groups() for m in imported))==1 and int(imported[0][1])>0 and int(imported[0][2],16)%16384==0
    handle=imported[0][1];assert all(c[1]==handle for c in cleared) and all(r[1]==handle for r in retired)
    assert all(0<int(r[2])<=s for r,s in zip(retired,sequences))
    tiled = None
    if tiled_revision:
        from depth_tiled_evidence import plane,verify_groups
        groups=[[plane(handle,imported[0][2],252,1,128,2 if phase<25 or phase==28 else 1)] for phase in range(29)]
        tiled=verify_groups(block,groups,tiled_revision)
    result=dict(complete_samples=1,incomplete_samples=0,phases=29,checked_bytes=74240,guard_bytes=38744,read_bytes=26100,
                records=rows,original_depth_handle=int(handle),original_GPU_address=imported[0][2],original_GPU_offset=64,
                original_depth32_pages_compare_load_clear_and_padding_verified=True,
                GPU_fences_strictly_increasing=True,final_allocated=0,
                stencil_MSAA_and_full_UI_not_inferred=True)
    if tiled is not None:result['tiled_work_planes']=tiled
    return result
