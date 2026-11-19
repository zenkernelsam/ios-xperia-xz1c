"""Original padded attachments and repeated Clear reuse, bound to one LIVE scope."""
import re
from air_render_evidence import fnv


def expected(index, phase):
    raw=bytearray([0xa0+index]*16384)
    for y in range(9):
        for x in range(17):
            inside=3<=x<14 and 2<=y<7
            red=(index*255+4)//8 if inside else 255 if phase^(index&1) else 0
            green=64 if inside else 0 if phase else 255
            blue=128 if inside else 255 if phase else 0
            pixel=bytes((red,green,blue,255) if index&1 else (blue,green,red,255))
            offset=64+y*128+x*4;raw[offset:offset+4]=pixel
    return fnv(raw)


def verify(text, token, fixture):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)]
    starts=list(re.finditer(r'SWEET CLEAR CACHE begin phases=2 targets=8 width=17 height=9 pitch=128 offset=64 original_pages=1',scope))
    assert len(starts)==1 and not re.search(r'SWEET CLEAR CACHE END result=[1-9]',scope)
    block=scope[starts[0].end():]
    end=re.search(r'SWEET CLEAR CACHE END result=0 phases=2 targets=8 checked_bytes=262144 final_allocated=0 original_clear=1 GPU_only=1',block)
    assert end;block=block[:end.start()]
    assert 'SWEET CLEAR CACHE input library_sha256='+fixture['library_sha256'] in block
    phases=list(re.finditer(r'SWEET CLEAR CACHE phase=([01]) sequence=(\d+) operations=1 checked_bytes=131072 mismatches=0 status=4 error=0 valid=1',block))
    buffers=list(re.finditer(r'SWEET CLEAR CACHE buffer phase=([01]) index=([0-7]) hash=([0-9a-f]{16}) original_pages=1',block))
    assert [int(p[1]) for p in phases]==[0,1]
    assert 0<int(phases[0][2])<int(phases[1][2])
    assert [(int(b[1]),int(b[2])) for b in buffers]==[(p,i) for p in range(2) for i in range(8)]
    assert [b[3] for b in buffers]==[expected(i,p) for p in range(2) for i in range(8)]
    clears=list(re.finditer(r'SWEET METAL CLEAR shader format=(70|80) slot=0 reused=([01]) cached=1 result=0 original_shader=1 original_clear=1',block))
    assert len(clears)==16 and [int(c[1]) for c in clears]==[80,70]*8
    assert [int(c[2]) for c in clears]==[0]+[1]*15
    for phase in range(2):
        previous=phases[phase-1].end() if phase else 0
        assert all(previous<b.start()<phases[phase].start() for b in buffers[phase*8:phase*8+8])
        assert all(previous<c.start()<buffers[phase*8].start() for c in clears[phase*8:phase*8+8])
    assert scope.count('SWEET CLEAR CACHE begin ')==1 and scope.count('SWEET CLEAR CACHE END ')==1
    assert scope.count('SWEET CLEAR CACHE buffer ')==16 and scope.count('SWEET CLEAR CACHE phase=')==2
    return dict(complete_samples=1,incomplete_samples=0,samples=[dict(phases=2,targets=8,checked_bytes=262144,
        original_padding_and_colours_equal=True,reused_Clear_shaders=15,first_Clear_compiled=True,
        final_allocated=0,original_pages=True)],full_UI_or_FPS_not_inferred=True)
