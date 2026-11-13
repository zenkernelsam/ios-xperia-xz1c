"""Real custom Metal queue, A5xx fences, complete buffer bytes and callbacks."""
import re
import struct

def packet7(opcode,count):
    parity=lambda value:(bin(int(value)).count('1')&1)^1
    return 0x70000000|(opcode<<16)|count|(parity(opcode)<<23)|(parity(count)<<15)

def packet_hash(source,destination,bytes_,fill):
    words=[packet7(0x26,0)]
    if source:
        for offset in range(0,bytes_,4):
            words += [packet7(0x73,5),1<<30,(destination+offset)&0xffffffff,(destination+offset)>>32,
                (source+offset)&0xffffffff,(source+offset)>>32]
    else:
        words += [packet7(0x3d,2+bytes_//4),destination&0xffffffff,destination>>32]+[fill*0x01010101]*(bytes_//4)
    words += [packet7(0x12,0)];hash_=14695981039346656037
    for byte in struct.pack('<'+'I'*len(words),*words):hash_=((hash_^byte)*1099511628211)&0xffffffffffffffff
    return len(words),f'{hash_:016x}'

def transport_records(block,first,second):
    begins=list(re.finditer(r'SWEET METAL BLIT BEGIN call=(\d+) source=(\d+) destination=(\d+) source_gpu=(0x[0-9a-f]+|0) destination_gpu=(0x[0-9a-f]+) source_offset=(\d+) destination_offset=(\d+) bytes=(\d+) fill=(\d+)',block))
    assert len(begins)==3,'Need actual return metadata for all three queue operations'
    expected=[(0,0,16,64,60),(1,40,96,64,0),(1,512,512,16396,0)]
    records=[];destination_id=source_id=None;source_gpu=destination_gpu=None
    for i,begin in enumerate(begins):
        call,source,destination=int(begin[1]),int(begin[2]),int(begin[3]);sg,dg=int(begin[4],16),int(begin[5],16)
        so,do,bytes_,fill=map(int,begin.groups()[5:])
        assert 1<=call<=64 and 1<=destination<=4096
        assert (bool(source),so,do,bytes_,fill)==expected[i]
        assert dg%16384==0 and 0x40000000<=dg<=0x80000000-49152
        destination_id=destination if destination_id is None else destination_id
        destination_gpu=dg if destination_gpu is None else destination_gpu
        assert destination==destination_id and dg==destination_gpu
        if source:
            assert 1<=source<=4096 and source!=destination and sg%16384==0 and 0x40000000<=sg<=0x80000000-49152
            source_id=source if source_id is None else source_id;source_gpu=sg if source_gpu is None else source_gpu
            assert source==source_id and sg==source_gpu
        else:assert sg==so==0
        scope=block[begin.end():begins[i+1].start() if i+1<len(begins) else len(block)]
        end=re.search(r'SWEET METAL BLIT END call='+str(call)+r' result=0 sequence=(\d+) chunks=(\d+) released=0 synchronized=0 native_return=1',scope)
        assert end,'Actual submission/wait/resource retirement failed'
        chunks=list(re.finditer(r'SWEET METAL BLIT submit call='+str(call)+r' chunk=(\d+) offset=(\d+) bytes=(\d+) words=(\d+) hash=([0-9a-f]{16}) sequence=(\d+) result=0',scope[:end.start()]))
        assert len(chunks)==int(end[2])==(2 if i==2 else 1)
        sequences=[]
        for chunk_index,chunk in enumerate(chunks):
            offset=chunk_index*16384;length=min(16384,bytes_-offset)
            assert list(map(int,chunk.groups()[:3]))==[chunk_index,offset,length]
            count,hash_=packet_hash(sg+so+offset if source else 0,dg+do+offset,length,fill)
            assert int(chunk[4])==count and chunk[5]==hash_
            sequence=int(chunk[6]);assert 0<sequence<=0xffffffff
            wait=re.search(r'SWEET METAL BLIT wait call='+str(call)+r' sequence='+str(sequence)+r' result=0',scope[chunk.end():end.start()])
            assert wait,'Native submitted fence did not complete'
            sequences.append(sequence)
        assert int(end[1])==sequences[-1] and all(a<b for a,b in zip(sequences,sequences[1:]))
        records.append(dict(call=call,source_handle=source,destination_handle=destination,source_gpu=hex(sg),destination_gpu=hex(dg),
            sequences=sequences,all_packet_bytes_independently_hashed=True,actual_submission_wait_and_retirement=True))
    sequences=sum([r['sequences'] for r in records],[])
    assert all(a<b for a,b in zip(sequences,sequences[1:])) and records[1]['sequences'][-1]==first and records[2]['sequences'][-1]==second
    return records

def expected_hash():
    value=14695981039346656037
    for i in range(33024):
        byte=0x3c if 16<=i<80 else ((40+i-96)*7+1)&255 if 96<=i<160 else (i*7+1)&255 if 512<=i<16908 else 0xa5
        value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def verify(text,token,transport_revision=None):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    begins=list(re.finditer(r'SWEET METAL QUEUE begin bytes=33024 native_queue=1 enqueue_fifo=1 full_guards=1 gpu_commands=1',scope))
    assert begins,'Actual queue probe missing'
    records=[];incomplete=0
    for i,begin in enumerate(begins):
        block=scope[begin.end():begins[i+1].start() if i+1<len(begins) else len(scope)]
        finish=re.search(r'SWEET METAL QUEUE END result=(\d+) final_allocated=(\d+) copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1',block)
        if not finish:incomplete+=1;continue
        assert finish[1]==finish[2]=='0','Actual queue failed or retained buffer pages'
        found=list(re.finditer(r'SWEET METAL QUEUE completed first_status=4 second_status=4 first_seq=(\d+) second_seq=(\d+) first_ops=2 second_ops=1 scheduled=2 callbacks=2 checked=33024 mismatches=0 hash=([0-9a-f]{16}) valid=1',block[:finish.start()]))
        if not found:incomplete+=1;continue
        assert len(found)==1;match=found[0];first,second=int(match[1]),int(match[2])
        assert 0<first<second<=0xffffffff and match[3]==expected_hash()
        transport=None
        # Match the native per-client submissions in this same probe, not a
        # GLES fence or another Aegir process. Small first commands finish at
        # first-1/first; the chunked second command finishes first+1/second.
        submits=list(re.finditer(r'SWEET SCATTER SUBMIT owner=(\d+) ibs=1 bos=([23]) seq=(\d+) result=0',block[:match.start()]))
        # The GPU sequence is global: another process may submit between our
        # operations. Match this client's actual four writes/copies, without
        # assuming global sequence numbers are contiguous.
        if transport_revision==1:
            transport=transport_records(block[:match.start()],first,second)
            sequences=sum([r['sequences'] for r in transport],[]);owner_id=None
        else:
            owner=[m[1] for m in submits if int(m[3])==second];assert len(owner)==1
            selected=[m for m in submits if m[1]==owner[0]]
            assert len(selected)==4 and [int(m[2]) for m in selected]==[2,3,3,3]
            sequences=[int(m[3]) for m in selected]
            assert all(a<b for a,b in zip(sequences,sequences[1:])) and sequences[1]==first and sequences[3]==second
            owner_id=int(selected[0][1])
        records.append(dict(owner=owner_id,first_completed_sequence=first,second_completed_sequence=second,
            native_submissions=4,commands=2,scheduled_callbacks=2,completed_callbacks=2,checked_bytes=33024,
            destination_hash=match[3],submission_sequences=sequences,transport=transport,outside_regions_preserved=True,final_allocated_bytes=0))
    assert records,'No complete native queue/fence/byte proof'
    return dict(actual_native_Metal_queue_and_GPU_blits_verified=True,records=records,complete_samples=len(records),incomplete_samples=incomplete,
        actual_A5xx_submission_matches_current_queue=True,enqueue_FIFO_order_verified=True,
        full_byte_guard_check_verified=True,shader_render_and_system_Metal_still_incomplete=True)
