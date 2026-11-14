"""Original constant values and frozen specialized GPU results, scoped by token."""
import re,struct

def expected_hash(phase):
    total=sum((13,29,37)[:phase+1]);value=14695981039346656037
    for byte in struct.pack('<80I',*((((i*0x9e3779b1)&0xffffffff)^0x434f4e53)+(total if i<64 else 0)&0xffffffff for i in range(80))):
        value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    starts=list(re.finditer(r'SWEET AIR CONSTANTS begin library_bytes=(\d+) standard_MTLB=1 native_constant_values=1 original_buffer=1',scope));assert starts
    records=[];incomplete=0
    for index,start in enumerate(starts):
        assert int(start[1])==fixture['library_bytes']
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)]
        end=re.search(r'SWEET AIR CONSTANTS END result=(\d+) phases=3 invocations=192 checked_words=240 guard_checks=48 final_allocated=(\d+) standard_Metal=1',block)
        if not end:incomplete+=1;continue
        assert end[1]==end[2]=='0','Specialized GPU bytes/lifetime failed';block=block[:end.start()]
        assert 'SWEET AIR CONSTANTS input library_sha256='+fixture['library_sha256'] in block
        assert 'SWEET AIR CONSTANTS rejection required=1 missing_error=1 wrong_type=1 native_values_class=MTLFunctionConstantValuesInternal valid=1' in block
        rows=list(re.finditer(r'SWEET AIR CONSTANTS phase=([0-2]) step=(13|29|37) total=(13|42|79) status=4 error=0 sequence=(\d+) operations=1 callback_count=([1-3]) checked_words=80 guard_words=16 mismatches=0 hash=([0-9a-f]{16}) frozen_after_reset=1 valid=1',block))
        if len(rows)<3:incomplete+=1;continue
        assert [tuple(map(int,row.group(1,2,3,5))) for row in rows]==[(0,13,13,1),(1,29,42,2),(2,37,79,3)]
        sequences=[int(row[4]) for row in rows];assert sequences==sorted(set(sequences)) and sequences[0]>0
        assert all(row[6]==expected_hash(int(row[1])) for row in rows)
        binds=re.findall(r'SWEET AIR MESA binding index=0 handle=(\d+) gpu=(0x[0-9a-f]+) cpu=(0x[0-9a-f]+) offset=0 bytes=320 logical=320 original_pages=1',block)
        assert len(binds)==3 and len(set(binds))==1 and int(binds[0][1],16)%16384==int(binds[0][2],16)%16384==0
        dispatches=re.findall(r'SWEET AIR MESA dispatch groups=1,1,1 local=64,1,1 result=0 sequence=(\d+) original_buffers=1',block);assert list(map(int,dispatches))==sequences
        compiled=re.findall(r'SWEET AIR MESA compiled stage=compute program=(\d+) buffer_mask=1 write_mask=1 source_bytes=(\d+) source_hash=([0-9a-f]{16}) linked=1 robust=1',block)
        assert len(compiled)==3 and len({row[2] for row in compiled})==3
        records.append(dict(sequences=sequences,steps=[13,29,37],total=[13,42,79],hashes=[row[6] for row in rows],original_handle=int(binds[0][0]),
            original_GPU_address=binds[0][1],original_CPU_address=binds[0][2],compiled_ESSL_hashes=[row[2] for row in compiled],checked_words=240,guard_words=48,final_allocated=0))
    assert records,'No complete frozen native constants GPU proof'
    return dict(actual_native_function_constants_GPU_verified=True,complete_samples=len(records),incomplete_samples=incomplete,
        required_type_name_index_range_reset_verified=True,original_buffer_pages_held=True,records=records,system_Metal_and_complete_UI_pending=True)
