"""Verify original native drawable pixels and GPU handoff, without optical claims."""
import re,struct,math
BEGIN='SWEET PRESENT PROBE BEGIN phases=5 width=17 height=9 original_CAMetalLayer=1 original_drawable=1'
END='SWEET PRESENT PROBE END result=0 phases=5 checked_pixels=765 final_allocated=0 original_drawables=1 display_completion_unverified=1'
ABI='SWEET PRESENT PROBE ABI valid=1 QuartzCore_uuid=9e40b2eb260c3af1ae45a1a551a8c1b9 original_classes=1'
PAGE=re.compile(r'SWEET PRESENT PROBE PAGE phase=(\d+) drawable=([0-9a-f]{16}) surface=(\d+) handle=(\d+) GPU=(0x[0-9a-f]+) pitch=(\d+) bytes=(\d+) framebuffer_only=1 usage=(\d+) valid=1')
REQUEST=re.compile(r'SWEET PRESENT PROBE REQUEST phase=(\d+) mode=(\d+) path=(command|transaction_wait) time_bits=([0-9a-f]{16})')
PHASE=re.compile(r'SWEET PRESENT PROBE PHASE index=(\d+) valid=1 sequence=(\d+) operations=1 checked_pixels=153 guard_bytes=(\d+) mismatches=0')
HASH=re.compile(r'SWEET PRESENT PROBE HASH index=(\d+) raw=([0-9a-f]{16}) pixels=([0-9a-f]{16}) padding=([0-9a-f]{16})')
NATIVE=re.compile(r'SWEET PRESENT PROBE NATIVE phase=(\d+) presented_time_bits=([0-9a-f]{16}) optical_unverified=1')
def fnv(data):
    v=14695981039346656037
    for byte in data:v=((v^byte)*1099511628211)&0xffffffffffffffff
    return f'{v:016x}'
def expected(pitch):
    assert 68<=pitch<=4096 and pitch%64==0
    raw=bytearray([0xa5]*pitch*9);pixels=bytearray()
    for y in range(9):
        for x in range(17):
            value=bytes((191,128,64,255) if 3<=x<14 and 2<=y<7 else (255,0,255,255));at=y*pitch+x*4;raw[at:at+4]=value;pixels+=value
    return fnv(raw),fnv(pixels),fnv(bytes([0xa5]*(pitch-68)*9))
def f64(bits):return struct.unpack('<d',bytes.fromhex(bits)[::-1])[0]
def verify(text,token,fixture):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1;scope=text[text.index(live):text.index(ack)]
    assert scope.count(BEGIN)==scope.count('SWEET PRESENT PROBE BEGIN ')==1 and scope.count(END)==scope.count('SWEET PRESENT PROBE END ')==1
    assert scope.index(BEGIN)<scope.index(END);block=scope[scope.index(BEGIN):scope.index(END)]
    assert block.count(ABI)==1 and block.count('SWEET PRESENT PROBE INPUT fixture_sha256='+fixture['library_sha256'])==1
    pages=list(PAGE.finditer(block));requests=list(REQUEST.finditer(block));phases=list(PHASE.finditer(block));hashes=list(HASH.finditer(block));natives=list(NATIVE.finditer(block))
    assert len(pages)==len(requests)==len(phases)==len(hashes)==len(natives)==5
    assert block.count('SWEET PRESENT PROBE ')==3+5*6+1,'Unknown, failed, duplicate or truncated original drawable record'
    assert [int(m[1]) for m in pages]==[int(m[1]) for m in requests]==[int(m[1]) for m in phases]==[int(m[1]) for m in hashes]==[int(m[1]) for m in natives]==list(range(5))
    records=[];previous=0
    for index,(page,request,phase,hash_,native) in enumerate(zip(pages,requests,phases,hashes,natives)):
        identity=page[2];surface=int(page[3]);handle=int(page[4]);gpu=int(page[5],16);pitch=int(page[6]);bytes_=int(page[7]);usage=int(page[8]);sequence=int(phase[2]);guards=int(phase[3])
        assert int(identity,16)>0 and surface>0 and handle>0 and 0x40000000<=gpu<0x80000000 and gpu%64==0
        assert bytes_==pitch*9 and usage in (0,4) and guards==(pitch-68)*9 and sequence>previous
        assert tuple(hash_.group(n) for n in (2,3,4))==expected(pitch),'Original drawable GPU pixels or padding differ'
        assert page.end()<request.start()<phase.start()<hash_.start()<native.start() and (index==4 or native.end()<pages[index+1].start())
        assert block.count(f'SWEET PRESENT PROBE CPU phase={index} framebuffer_read_rejected=1')==1
        mode=index if index<4 else 0;assert int(request[2])==mode and request[3]==('transaction_wait' if index==4 else 'command')
        time=f64(request[4]);assert math.isfinite(time) and time>=0
        if index==1:assert time>0
        else:assert time==(0.0625 if index==2 else 0)
        segment=block[page.start():phase.start()]
        imports=list(re.finditer(r'SWEET AIR RENDER texture handle='+str(handle)+r' width=17 height=9 pitch='+str(pitch)+r' format=80 offset=0 gpu=(0x[0-9a-f]+) original_pages=1',segment))
        assert len(imports)==1 and imports[0][1]==page[5]
        retired=list(re.finditer(r'SWEET METAL MRT retired index=0 handle='+str(handle)+r' format=80 result=0 sequence=(\d+) original_pages=1',segment))
        assert len(retired)==1 and int(retired[0][1])==sequence
        draw=list(re.finditer(r'SWEET AIR RENDER draw width=17 height=9 primitive=3 first=0 count=3 instances=1 target_handle='+str(handle)+r' result=0 sequence=(\d+) original_pages=1',segment))
        assert len(draw)==1 and int(draw[0][1])==sequence
        publication=list(re.finditer(r'SWEET DRAWABLE PUBLISH surface='+str(surface)+r' handle='+str(handle)+r' sequence='+str(sequence)+r' result=0 original_surface=1',segment))
        assert len(publication)==1 and draw[0].end()<publication[0].start()
        if index<4:
            begins=list(re.finditer(r'SWEET PRESENT BEGIN mode='+str(mode)+r' drawable='+identity+r' surface='+str(surface)+r' handle='+str(handle)+r' sequence='+str(sequence)+r' time_bits='+request[4]+r' original_object=1',segment))
            ends=list(re.finditer(r'SWEET PRESENT END mode='+str(mode)+r' surface='+str(surface)+r' sequence='+str(sequence)+r' result=0 original_present=1 display_completion_unverified=1',segment))
            assert len(begins)==len(ends)==1 and publication[0].end()<begins[0].start()<ends[0].start()
        else:assert segment.count('SWEET PRESENT PROBE DIRECT phase=4 original_wait_then_present=1')==1 and 'SWEET PRESENT BEGIN ' not in segment
        presented=f64(native[2]);assert math.isfinite(presented) and presented>=0
        records.append(dict(phase=index,mode=mode,path=request[3],sequence=sequence,IOSurface_id=surface,handle=handle,GPU=page[5],pitch=pitch,checked_bytes=bytes_,guard_bytes=guards,pixels_hash=hash_[3],native_presented_time_bits=native[2]));previous=sequence
    assert block.count('SWEET PRESENT BEGIN ')==block.count('SWEET PRESENT END ')==4
    return dict(complete_samples=1,incomplete_samples=0,phases=5,checked_pixels=765,records=records,
        original_CAMetalLayer_drawable_IOSurface_GPU_pixels_and_padding_verified=True,original_present_after_GPU_retirement_and_transaction_wait_path_verified=True,
        native_display_completion_optical_and_original_UI_not_proven=True,final_allocated=0,raw_is_actual_device_evidence_only_when_called_from_verified_trial=True)
