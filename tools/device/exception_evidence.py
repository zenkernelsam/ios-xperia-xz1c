"""Bounded real exception records from the current boot; no exception repair claim."""
import re

def observer_records(scope):
    """Join bare console-wrap fragments only inside one observer record.

    A new tee/log record is always a boundary. Lost frames are never recovered
    from another process, its first-throw text, or a later exception.
    """
    records=[];pending=None
    for line in scope.splitlines():
        match=re.match(r'^(?:\|AegirPoster\| )?(SWEET EXCEPTION .*)$',line)
        if match:
            if pending is not None:records.append(pending)
            pending=match[1]
        elif pending is not None and line and not re.match(r'^(?:\||SWEET|CRASH|Process|exec|IOK|IOMemory|MESA|Backtrace|\s)',line):
            pending+=line
            assert len(pending)<=512,'Observer record exceeds its actual write bound'
        else:
            if pending is not None:records.append(pending)
            pending=None
    if pending is not None:records.append(pending)
    return '\n'.join(records)

def inspect_scope(scope):
    scope=observer_records(scope)
    installed=re.findall(r'SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1',scope)
    assert installed,'AegirPoster observer did not actually load'
    begins=list(re.finditer(r'SWEET EXCEPTION begin call=(\d+) class=(\S+) name="([^"]*)" reason="([^"]*)" observational=1',scope))
    assert begins,'No actual exception metadata captured'
    records=[];incomplete=[]
    # Each new helper process restarts its observation counter. Preserve that
    # sequence instead of pretending counters are unique across process restarts.
    for index,begin in enumerate(begins):
        call=int(begin[1]);assert 1<=call<=16
        finish_limit=begins[index+1].start() if index+1<len(begins) else len(scope)
        text=scope[begin.end():finish_limit]
        ends=list(re.finditer(r'SWEET EXCEPTION end call='+str(call)+r' frames=(\d+) original_chain=1',text))
        if not ends:
            incomplete.append(dict(call=call,reason='end_record_missing'));continue
        assert len(ends)==1;end=ends[0];frames=[]
        for frame in re.finditer(r'SWEET EXCEPTION frame call='+str(call)+r' index=(\d+) pc=(0x[0-9a-f]+) base=(0x[0-9a-f]+|0x0) image=(\S+) symbol=(\S+)',text[:end.start()]):
            frames.append(dict(index=int(frame[1]),pc=frame[2],image_base=frame[3],image=frame[4],symbol=frame[5]))
        declared=int(end[1]);assert 1<=declared<=16 and len(frames)<=declared
        indices=[f['index'] for f in frames]
        assert indices==sorted(set(indices)) and all(0<=i<declared for i in indices)
        if len(frames)<declared:
            incomplete.append(dict(call=call,reason='frame_records_missing',declared_frames=declared,
                captured_frame_indices=indices));continue
        assert [f['index'] for f in frames]==list(range(len(frames)))
        records.append(dict(call=call,exception_class=begin[2],name=begin[3],reason=begin[4],frames=frames,
            original_exception_chain_continued=True))
    assert records,'No complete actual exception record'
    return dict(observer_load_count=len(installed),records=records,incomplete_records=incomplete,
        observed_exception_metadata_only=True,exception_cause_not_yet_repaired=True)

def verify_renderer(scope):
    installed=re.findall(r'SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=([Qq]) device_object=1 original_forwarded=1',scope)
    assert installed,'Actual renderer ABI hook absent'
    matches=re.findall(r'SWEET METAL uniform call=(\d+) index=(\d+) device_nil=([01]) device_class=(\S+) new_buffer_method=([01]) metadata_only=1 original_called=1',scope)
    assert matches,'No actual renderer device observation'
    rows=[]
    for call,index,absent,type_,supports in matches:
        call=int(call);index=int(index);absent=int(absent);supports=int(supports)
        assert 1<=call<=32 and 0<=index<3
        assert (not absent and type_!='nil') or (absent and type_=='nil' and not supports)
        rows.append(dict(call=call,frame_buffer_index=index,device_nil=bool(absent),device_class=type_,new_buffer_method=bool(supports)))
    return dict(actual_renderer_ABI_verified=True,device_observations=rows,
        all_observed_devices_nil=all(r['device_nil'] for r in rows),metadata_only=True,
        original_renderer_forwarded=True,no_Metal_buffer_or_rendering_claim=True)

def verify(text,token,require_renderer=False):
    start=text.index('SWEET LOOP LIVE proto=1 token='+token)
    end=text.index('SWEET LOOP ACK FASTBOOT token='+token)
    scope=text[start:end]
    assert 'SWEET ENV: AegirPoster += DYLD_INSERT_LIBRARIES=/dev/sglram/e' in scope
    report=inspect_scope(scope)
    if require_renderer:report['renderer']=verify_renderer(scope)
    return report
