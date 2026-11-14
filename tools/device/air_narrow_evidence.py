"""Independent signed/unsigned packed8/16 scalar/vector GPU bytes and guards."""
import re,struct
from air_render_evidence import fnv
def values(phase):
    output=bytearray([0xa5]*16384);shorts=bytearray([0xb6]*16384);chars=bytearray([0xc7]*16384)
    v16=[0,1,32767,32768,65535,12345,50000,65280];v8=[0,1,127,128,255,39,192,240]
    for i in range(64):
        a,b=[v16[(i*3+c*5+phase*2)&7] for c in range(2)];c,d=[v8[(i*5+n*3+phase)&7] for n in range(2)]
        shorts[64+i*4:68+i*4]=struct.pack('<2H',a,b);chars[64+i*2:66+i*2]=bytes((c,d))
        sa=a-65536 if a>=32768 else a;sb=b-65536 if b>=32768 else b;sc=c-256 if c>=128 else c;sd=d-256 if d>=128 else d
        out=(sa,a,sd,d,sa,sb,a,b,sc,sd,c,d);at=64+i*48;output[at:at+48]=struct.pack('<12f',*out)
    return [fnv(p) for p in (output,shorts,chars)]
def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET AIR NARROW END result=[1-9]',scope)
    starts=list(re.finditer(r'SWEET AIR NARROW begin bits=8,16 signs=signed,unsigned lanes=1,2 phases=2 invocations=64 original_buffers=3',scope));assert starts
    records=[];incomplete=0
    for i,start in enumerate(starts):
        block=scope[start.end():starts[i+1].start() if i+1<len(starts) else len(scope)]
        end=re.search(r'SWEET AIR NARROW END result=(\d+) phases=2 checked_bytes=98304 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','0');block=block[:end.start()]
        assert 'SWEET AIR NARROW input library_sha256='+fixture['library_sha256'] in block
        phases=list(re.finditer(r'SWEET AIR NARROW phase=([01]) sequence=(\d+) operations=1 status=4 error=0 checked_bytes=49152 mismatches=0 valid=1',block))
        buffers=list(re.finditer(r'SWEET AIR NARROW buffer phase=([01]) index=([012]) bytes=16384 hash=([0-9a-f]{16}) original_pages=1',block))
        if len(phases)<2 or len(buffers)<6:incomplete+=1;continue
        assert [int(p[1]) for p in phases]==[0,1] and int(phases[1][2])>int(phases[0][2])>0
        assert [(int(p[1]),int(p[2])) for p in buffers]==[(phase,b) for phase in (0,1) for b in range(3)]
        for row in buffers:assert row[3]==values(int(row[1]))[int(row[2])],'Dynamic packed integer sign/shape/padding differs'
        bindings=re.findall(r'SWEET AIR MESA binding index=([012]) handle=(\d+) gpu=(0x[0-9a-f]+) cpu=(0x[0-9a-f]+) offset=64 bytes=(3136|320|192) logical=(3200|384|256) original_pages=1',block)
        assert len(bindings)==6
        for index in (0,1,2):
            bound=[p for p in bindings if int(p[0])==index];assert len(bound)==2 and bound[0]==bound[1]
            assert tuple(map(int,bound[0][4:]))==[(3136,3200),(320,384),(192,256)][index]
            assert 0<int(bound[0][1])<=4096 and int(bound[0][2],16)&16383==int(bound[0][3],16)&16383==0
        records.append(dict(sequences=[int(p[2]) for p in phases],buffer_hashes=[p[3] for p in buffers],checked_bytes=98304,final_allocated=0,
            scalar_vector_signed_unsigned8_16_original_bits_verified=True,all_input_output_padding_checked=True))
    assert records,'No complete original-page dynamic narrow conversion sample'
    return dict(actual_dynamic_narrow_integer_GPU_conversion_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,original_UI_still_requires_separate_acceptance=True)
