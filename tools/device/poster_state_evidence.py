"""Decode changed original environment returns in one LIVE/ACK interval."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct

APIS = {'preview':'B','snapshot':'B','low_light':'B','backlight':'D',
        'linear_backlight':'D','unlock':'D','min_size':'S','desired_size':'S',
        'renderer_env':'I','env_init':'E','env_update':'E'}
CONFIG = re.compile(r'SWEET PST CONFIG pid=(\d+) revision=1 apis=11 slots=256 max=512 original_returns=1 getter_calls=0 GPU_work=0')
VALUE = re.compile(r'SWEET PST VALUE pid=(\d+) sample=(\d+) api=(\w+) receiver=([0-9a-f]{16}) kind=([BDSIE]) outcome=([RT]) ticks=([0-9a-f]{16})')
BITS = re.compile(r'SWEET PST BITS pid=(\d+) sample=(\d+) first=([0-9a-f]{16}) second=([0-9a-f]{16})')
LIMIT = re.compile(r'SWEET PST LIMIT pid=(\d+) reason=(busy|slots|records) original_forwarded=1')


def inspect(raw, token):
    if not re.fullmatch('[0-9a-f]{16}',token):
        raise ValueError('Invalid current token')
    live='SWEET LOOP LIVE proto=1 token='+token
    ack='SWEET LOOP ACK FASTBOOT token='+token
    if raw.count(live)!=1 or raw.count(ack)!=1 or raw.index(live)>=raw.index(ack):
        raise ValueError('Need one current LIVE/ACK interval')
    scope=raw[raw.index(live):raw.index(ack)]
    patterns=(CONFIG,VALUE,BITS,LIMIT)
    matches=sorted((m.start(),i,m) for i,p in enumerate(patterns) for m in p.finditer(scope))
    if len(matches)!=scope.count('SWEET PST '):
        raise ValueError('Unknown or truncated original state record')
    pids=set();records={};limits=[]
    for _,kind,m in matches:
        pid=int(m[1])
        if pid<=0:raise ValueError('Invalid state process')
        if kind==0:
            if pid in pids:raise ValueError('Repeated original state configuration')
            pids.add(pid);continue
        if pid not in pids:raise ValueError('State record before original configuration')
        if kind==3:
            if (pid,m[2]) in limits:raise ValueError('Repeated state limit')
            limits.append((pid,m[2]));continue
        sample=int(m[2]);key=pid,sample
        if not 1<=sample<=512:raise ValueError('State sample outside original bound')
        if kind==1:
            if key in records or m[3] not in APIS or APIS[m[3]]!=m[5] or int(m[4],16)==0:
                raise ValueError('Invalid, foreign or repeated original state sample')
            records[key]={'pid':pid,'sample':sample,'api':m[3],'receiver_identity':m[4],
                          'kind':m[5],'outcome':m[6],'ticks':m[7],'complete':False}
        else:
            if key not in records or records[key]['complete']:raise ValueError('State bits missing/repeated header')
            r=records[key];r.update(first_bits=m[3],second_bits=m[4],complete=True)
            if r['kind'] in ('B','D','I') and int(m[4],16):raise ValueError('Nonzero scalar state padding')
            if r['kind']=='B' and int(m[3],16)>255:raise ValueError('Original BOOL state exceeds its byte ABI')
            if r['outcome']=='R':
                if r['kind']=='B':r['boolean_result']=bool(int(m[3],16))
                elif r['kind']=='D':
                    value=struct.unpack('<d',bytes.fromhex(m[3])[::-1])[0]
                    r.update(double_value=value if math.isfinite(value) else None,double_is_finite=math.isfinite(value))
                elif r['kind']=='S':
                    values=[struct.unpack('<d',bytes.fromhex(x)[::-1])[0] for x in (m[3],m[4])]
                    r.update(size_values=[x if math.isfinite(x) else None for x in values],size_is_finite=[math.isfinite(x) for x in values])
                elif r['kind']=='I':r['return_identity']=m[3]
                else:r.update(renderer_identity=m[3],transition_identity=m[4])
    if not pids or not records or any(not r['complete'] for r in records.values()):
        raise ValueError('Missing complete original changed state capture')
    for pid in pids:
        ids={sample for p,sample in records if p==pid}
        if not ids or ids!=set(range(1,max(ids)+1)):raise ValueError('Missing original changed state sample')
        cache_keys={(r['api'],r['receiver_identity']) for r in records.values() if r['pid']==pid}
        if len(cache_keys)>256:raise ValueError('Original state cache bound exceeded')
    return {'revision':1,'token':token,'processes':sorted(pids),'records':list(records.values()),
            'limits':[{'pid':p,'reason':reason} for p,reason in limits],
            'complete_samples':len(records),'post_ACK_excluded':True,'original_getters_not_called':True,
            'exact_return_bits_kept':True,'coalesced_changed_states_not_total_calls':True,
            'bounded_state_capture_not_entire_boot_or_optical_GPU_proof':True}


def verify(directory):
    from scanout_trial_evidence import verify as verify_screen
    root=Path(directory);screen=verify_screen(root)
    row=json.loads((root/'results.json').read_bytes())[0];artifact=row['artifact']
    observer=artifact['exception_observer']
    if observer.get('quad_route_revision')!=5 or observer.get('poster_state_observer_revision')!=1:
        raise ValueError('Original compiled state observer binding differs')
    data=(root/'cycle-01/serial.raw.log').read_bytes()
    if hashlib.sha256(data).hexdigest()!=screen['raw_sha256']:raise ValueError('Original raw binding differs')
    result=inspect(data.decode('ascii','replace'),artifact['token'])
    result.update(source=str(root.resolve()),raw_sha256=screen['raw_sha256'],
        original_controller_result=row['result'],original_controller_reason=row['reason'],
        stage_not_promoted=True,device_operations=0)
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();result=verify(args.directory)
    with Path(args.output).open('x',encoding='utf-8',newline='\n') as f:json.dump(result,f,indent=2);f.write('\n')
    print(json.dumps({k:result[k] for k in ('token','original_controller_result','complete_samples','limits')}))
