"""Actual AIR input -> standard Metal -> ESSL/Mesa -> original GPU buffer."""
import re,struct

def expected_hash(phase):
    words=[(((i*0x9e3779b1)&0xffffffff)^0x54544149)+(7*(phase+1) if i<64 else 0) for i in range(80)]
    value=14695981039346656037
    for byte in struct.pack('<80I',*(w&0xffffffff for w in words)):value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    begins=list(re.finditer(r'SWEET AIR COMPUTE begin library_bytes=(\d+) standard_MTLB=1 standard_compute_encoder=1 original_buffer=1',scope))
    assert begins,'Actual AIR compute probe missing'
    records=[];incomplete=0
    for i,begin in enumerate(begins):
        assert int(begin[1])==fixture['library_bytes']
        block=scope[begin.end():begins[i+1].start() if i+1<len(begins) else len(scope)]
        finish=re.search(r'SWEET AIR COMPUTE END result=(\d+) phases=2 kernel_invocations=128 checked_words=160 guard_checks=32 final_allocated=(\d+) standard_Metal=1',block)
        if not finish:incomplete+=1;continue
        assert finish[1]==finish[2]=='0','Actual AIR kernel/lifetime failed'
        assert 'SWEET AIR COMPUTE input library_sha256='+fixture['library_sha256'] in block[:finish.start()]
        phases=list(re.finditer(r'SWEET AIR COMPUTE phase=([01]) status=4 error=0 sequence=(\d+) operations=1 callback_count=([12]) checked_words=80 guard_words=16 mismatches=0 hash=([0-9a-f]{16}) same_original_pages=1 valid=1',block[:finish.start()]))
        if len(phases)<2:incomplete+=1;continue
        assert [int(p[1]) for p in phases]==[0,1] and int(phases[1][2])>int(phases[0][2])>0
        for phase in phases:
            assert int(phase[3])==int(phase[1])+1 and phase[4]==expected_hash(int(phase[1]))
        compiled=re.search(r'SWEET AIR MESA compiled stage=compute program=(\d+) buffer_mask=1 write_mask=1 source_bytes=(\d+) source_hash=([0-9a-f]{16}) linked=1 robust=1',block[:phases[0].start()])
        assert compiled and int(compiled[1])>0 and int(compiled[2])>100
        binding=list(re.finditer(r'SWEET AIR MESA binding index=0 handle=(\d+) gpu=(0x[0-9a-f]+) cpu=(0x[0-9a-f]+) offset=0 bytes=320 logical=320 original_pages=1',block[:finish.start()]))
        assert len(binding)==2 and [m.groups() for m in binding[:1]]*2==[m.groups() for m in binding]
        assert int(binding[0][1])>0 and int(binding[0][2],16)%16384==int(binding[0][3],16)%16384==0
        dispatches=re.findall(r'SWEET AIR MESA dispatch groups=1,1,1 local=64,1,1 result=0 sequence=(\d+) original_buffers=1',block[:finish.start()])
        assert list(map(int,dispatches))==[int(p[2]) for p in phases]
        records.append(dict(library_sha256=fixture['library_sha256'],sequence=[int(p[2]) for p in phases],
            original_handle=int(binding[0][1]),GPU_original_address=binding[0][2],CPU_original_address=binding[0][3],
            destination_hashes=[p[4] for p in phases],checked_words=160,guard_words=32,final_allocated_bytes=0,
            translated_source_hash=compiled[3],translated_source_bytes=int(compiled[2])))
    assert records,'No complete current-boot AIR kernel and shared buffer proof'
    return dict(actual_MTLB_AIR_compute_GPU_path_verified=True,complete_samples=len(records),incomplete_samples=incomplete,records=records,
        original_Metal_buffer_pages_used=True,guards_checked=True,system_Metal_and_full_rendering_not_yet_achieved=True)
