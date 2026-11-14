"""Independent raw binary16 arithmetic, NaN class, GPU graphics and full guards."""
import math,re,struct
from air_render_evidence import fnv
def half_value(bits):
    e=(bits>>10)&31;m=bits&1023
    value=(math.nan if m else math.inf) if e==31 else math.ldexp(1+m/1024,e-15) if e else math.ldexp(m,-24)
    return -value if bits&32768 else value
def pack(value):
    if math.isnan(value):return 0x7e00
    try:return struct.unpack('<H',struct.pack('<e',value))[0]
    except OverflowError:return 0xfc00 if math.copysign(1,value)<0 else 0x7c00
def inputs(i,c,phase):
    if not i and not c:return 0x7e01
    values=[0,0x8000,1,0x8001,0x3ff,0x400,0x3c00,0x3c01,0x3555,0x7bfe,0x7bff,0x800,0x8400,0x7a00,0xfa00,0x3bff]
    return values[(i*3+c*5+phase*7)&15]
def full_value(i,phase):
    if not i:return math.inf
    values=[math.inf,-math.inf,1.00048828125,1.00146484375,65519.0,65520.0,-1.00048828125,2**-25,1.5*2**-24,-2**-25,
        .3333,-.3333,float.fromhex('0x1.ff8p-15'),2.001,100.01,-2.001]
    return struct.unpack('<f',struct.pack('<f',values[(i*5+phase*3)&15]))[0]
def expected(phase):
    out=bytearray([0xa5]*16384);input_=bytearray([0xb6]*16384);full=bytearray([0xc7]*16384);nans=[]
    for i in range(64):
        h=[inputs(i,c,phase) for c in range(4)];r=list(h)
        for c in range(4):
            added=pack(half_value(h[c])+half_value([0x1000,0x9000,0x3555,0xb400][c]));r.append(pack(half_value(added)*half_value([0x3800,0x4000,0x3e00,0x3c00][c])))
        n=pack(full_value(i,phase));r += [n,pack(half_value(n)+half_value(h[0])),pack(half_value(n)*half_value(h[1])),pack(half_value(h[2])/2)]
        for c,v in enumerate(r):
            if (v&0x7c00)==0x7c00 and (v&1023):r[c]=0x7e00;nans.append(i*12+c)
        out[64+i*24:88+i*24]=struct.pack('<12H',*r);input_[64+i*8:72+i*8]=struct.pack('<4H',*h);full[64+i*4:68+i*4]=struct.pack('<f',full_value(i,phase))
    return out,input_,full,nans
def graphics():
    target=bytearray([0xe7]*16384);vertices=bytearray([0xd4]*16384)
    for y in range(9):
        for x in range(17):at=128+y*128+x*4;target[at:at+4]=bytes((191,128,64,255))
    vertices[64:88]=struct.pack('<12H',0xbc00,0x3c00,0x3800,0x3c00,0x4200,0x3c00,0x3800,0x3c00,0xbc00,0xc200,0x3800,0x3c00)
    return fnv(target),fnv(vertices)
def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET AIR HALF END result=[1-9]',scope)
    begins=list(re.finditer(r'SWEET AIR HALF begin phases=2 invocations=64 raw16_storage=1 result_rounding=1 graphics=1 original_buffers=1',scope));assert begins
    records=[];incomplete=0
    for i,begin in enumerate(begins):
        block=scope[begin.end():begins[i+1].start() if i+1<len(begins) else len(scope)]
        end=re.search(r'SWEET AIR HALF END result=(\d+) checked_bytes=131072 phases=2 graphics=1 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','0');block=block[:end.start()]
        assert 'SWEET AIR HALF input library_sha256='+fixture['library_sha256'] in block
        phases=list(re.finditer(r'SWEET AIR HALF phase=([01]) sequence=(\d+) operations=1 status=4 error=0 checked_bytes=49152 mismatches=0 valid=1',block))
        buffers=list(re.finditer(r'SWEET AIR HALF buffer phase=([01]) index=([012]) raw_hash=([0-9a-f]{16}) original_pages=1',block))
        normalized=list(re.finditer(r'SWEET AIR HALF normalized phase=([01]) output_hash=([0-9a-f]{16}) nan_words=(\d+) finite_exact=1 NaN_class=1',block))
        draw=re.search(r'SWEET AIR HALF graphics sequence=(\d+) operations=1 status=4 error=0 checked_bytes=32768 pixels=153 mismatches=0 valid=1',block)
        hashes=re.search(r'SWEET AIR HALF graphics_hash target=([0-9a-f]{16}) vertices=([0-9a-f]{16}) original_pages=1',block)
        if len(phases)<2 or len(buffers)<6 or len(normalized)<2 or not draw or not hashes:incomplete+=1;continue
        assert [int(r[1]) for r in phases]==[0,1] and [int(r[1]) for r in normalized]==[0,1]
        sequences=[int(r[2]) for r in phases]+[int(draw[1])];assert sequences==sorted(set(sequences)) and sequences[0]>0
        assert [(int(r[1]),int(r[2])) for r in buffers]==[(p,b) for p in (0,1) for b in range(3)]
        for phase in (0,1):
            output,input_,full,nans=expected(phase);assert normalized[phase][2]==fnv(output) and int(normalized[phase][3])==len(nans)
            raw=bytearray(output);nan_records=re.findall(r'SWEET AIR HALF NaN phase='+str(phase)+r' word=(\d+) bits=([0-9a-f]{4})',block)
            assert [int(p[0]) for p in nan_records]==nans,'NaN index metadata incomplete or unexpected'
            for word,bits in nan_records:
                word=int(word);bits=int(bits,16);assert (bits&0x7c00)==0x7c00 and bits&1023
                raw[64+word*2:66+word*2]=struct.pack('<H',bits)
            assert [r[3] for r in buffers[phase*3:phase*3+3]]==[fnv(p) for p in (raw,input_,full)],'Actual binary16 raw result/input/guards differ'
        assert hashes.groups()==graphics(),'Half vertex/fragment or guard pixels differ'
        records.append(dict(sequences=sequences,checked_bytes=131072,compute_phases=2,graphics_pixels=153,finite_half_bits_exact=True,NaN_class_and_raw_bits_logged=True,
            target_hash=hashes[1],vertex_guard_hash=hashes[2],original_pages_and_all_guards_verified=True,final_allocated=0))
    assert records,'No complete real binary16 GPU arithmetic/graphics sample'
    return dict(actual_GPU_half_storage_per_result_rounding_and_graphics_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        original_UI_and_general_half_profile_still_pending=True)
