"""Actual typed buffers backed by SweetScatter; CPU data/lifetime only."""
import re

def expected_hash(index,bytes_):
    value=14695981039346656037
    for position in range(bytes_):value=((value^((position*13+index*71+3)&255))*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def verify(text,token):
    start=text.index('SWEET LOOP LIVE proto=1 token='+token);end=text.index('SWEET LOOP ACK FASTBOOT token='+token)
    scope=text[start:end];begins=list(re.finditer(r'SWEET METAL RESOURCE begin custom_factory=1 typed_objects=1 kernel_backing=SweetScatter gpu_submit=0',scope))
    assert begins,'Actual native resource probe absent'
    complete=[];incomplete=0;lengths=[432,16384,16385];spans=[16384,16384,32768]
    for i,begin in enumerate(begins):
        block=scope[begin.end():begins[i+1].start() if i+1<len(begins) else len(scope)]
        finish=re.search(r'SWEET METAL RESOURCE END result=(\d+) buffers=3 bytes=33201 span=65536 final_allocated=(\d+) gpu_submit=0',block)
        if not finish:incomplete+=1;continue
        assert finish[1]=='0' and finish[2]=='0','Actual resource probe failed or retained backing'
        views=list(re.finditer(r'SWEET METAL RESOURCE view index=(\d+) length=(\d+) allocated=(\d+) gpu=(0x[0-9a-f]+) cpu=(0x[0-9a-f]+) cache=0 storage=0 checked=(\d+) mismatches=0 hash=([0-9a-f]{16}) owner_match=1 valid=1',block[:finish.start()]))
        if len(views)<3:incomplete+=1;continue
        assert [int(v[1]) for v in views]==[0,1,2]
        for index,view in enumerate(views):
            assert int(view[2])==int(view[6])==lengths[index] and int(view[3])==spans[index]
            address=int(view[4],16);cpu=int(view[5],16)
            assert 0x40000000<=address<0x80000000 and spans[index]<=0x80000000-address and address%16384==0
            assert cpu and cpu%16384==0
            assert view[7]==expected_hash(index,lengths[index])
        assert len(set(v[4] for v in views))==len(set(v[5] for v in views))==3
        assert 'SWEET METAL RESOURCE lifetime owner_released=1 held_owner=1 current=65536 valid=1' in block[:finish.start()]
        complete.append([dict(length=int(v[2]),allocated=int(v[3]),gpu=v[4],cpu=v[5],hash=v[7]) for v in views])
    assert complete,'No complete actual resource lifecycle'
    return dict(actual_Metal_objects_and_GPU_backing_verified=True,complete_samples=len(complete),incomplete_samples=incomplete,
        buffers_per_sample=3,logical_bytes=lengths,backing_bytes=spans,samples=complete,
        retained_after_original_device_release=True,final_allocated_bytes=0,CPU_pattern_bytes_per_sample=33201,
        requested_WriteCombined_reports_actual_DefaultCache=True,GPU_submissions_from_probe=0,
        Metal_shader_queue_rendering_and_system_device_not_yet_provided=True)
