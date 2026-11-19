"""Pair real original request calls; missing draws stay missing, never repaired."""
import argparse,hashlib,json,math,re
from pathlib import Path

APIS=('function','function_constants','function_names','next_drawable','set_device','set_drawable_size')
ARGUMENTS=dict(function=3,function_constants=5,function_names=2,next_drawable=2,set_device=3,set_drawable_size=3)
METHOD=re.compile(r'SWEET METAL REQUEST METHOD pid=(\d+) api=(\w+) present=([01]) owned=([01]) installed=([01]) arguments=(\d+) exact_ABI=1')
CONFIG=re.compile(r'SWEET METAL REQUEST CONFIG pid=(\d+) installed=(\d+) expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1')
BEGIN=re.compile(r'SWEET METAL REQUEST BEGIN pid=(\d+) call=(\d+) api=(\w+) receiver=(\S+) argument_nil=([01]) argument_class=(\S+) original_forwarded=1')
END=re.compile(r'SWEET METAL REQUEST END pid=(\d+) call=(\d+) api=(\w+) outcome=(return|throw) returned_nil=([01]) return_class=(\S+) error_pointer=([01]) error_code=(-?\d+) original_behavior=1')
NAME=re.compile(r'SWEET METAL REQUEST NAME pid=(\d+) call=(\d+) bytes=(\d+) hex=(\S+) truncated=([01]) original_name_argument=1')
SIZE=re.compile(r'SWEET METAL REQUEST SIZE pid=(\d+) call=(\d+) width=(\S+) height=(\S+) original_arguments=1')
ABI=re.compile(r'SWEET METAL REQUEST ABI pid=(\d+) owner=(quad|renderer) selector=(\S+) present=([01]) types_hex=(\S+) truncated=([01]) metadata_only=1')

def inspect(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1;scope=text[text.index(live):text.index(ack)]
    methods={};configured={};records={};inventory=[]
    patterns=(METHOD,CONFIG,BEGIN,END,NAME,SIZE,ABI)
    matches=sorted([(m.start(),i,m) for i,p in enumerate(patterns) for m in p.finditer(scope)])
    assert len(matches)==scope.count('SWEET METAL REQUEST '),'Truncated or unknown request record'
    for _,kind,m in matches:
        pid=int(m[1]);assert pid>0
        if kind==0:
            api=m[2];assert api in APIS;entry=methods.setdefault(pid,{})
            assert pid not in configured and api not in entry
            entry[api]=dict(present=m[3]=='1',owned=m[4]=='1',installed=m[5]=='1',arguments=int(m[6]))
            assert not entry[api]['installed'] or (entry[api]['present'] and entry[api]['owned'] and entry[api]['arguments']==ARGUMENTS[api])
        elif kind==1:
            assert pid in methods and set(methods[pid])==set(APIS) and pid not in configured
            installed=int(m[2]);assert installed==sum(v['installed'] for v in methods[pid].values())
            configured[pid]=installed
        elif kind==6:
            assert pid in methods and pid not in configured
            types=m[5];raw=bytes.fromhex(types) if types!='none' else b'';assert len(raw)<=144
            assert bool(raw)==(m[4]=='1')
            inventory.append(dict(pid=pid,owner=m[2],selector=m[3],present=m[4]=='1',types=raw.decode('ascii'),truncated=m[6]=='1'))
        else:
            assert pid in configured
            key=(pid,int(m[2]));assert 1<=key[1]<=64
            if kind==2:
                assert key not in records and m[3] in APIS and methods[pid][m[3]]['installed']
                assert (m[5]=='1')==(m[6]=='nil')
                records[key]=dict(pid=pid,call=key[1],api=m[3],receiver=m[4],argument_nil=m[5]=='1',argument_class=m[6],complete=False)
            else:
                assert key in records;record=records[key];assert not record['complete']
                if kind==3:
                    assert m[3]==record['api'] and (m[5]=='1')==(m[6]=='nil')
                    assert not int(m[8]) or m[7]=='1'
                    if record['api'] in ('function','function_constants'):assert 'name' in record
                    if record['api']=='set_drawable_size':assert 'size' in record
                    record.update(complete=True,outcome=m[4],returned_nil=m[5]=='1',return_class=m[6],error_pointer=m[7]=='1',error_code=int(m[8]))
                elif kind==4:
                    assert record['api'] in ('function','function_constants') and 'name' not in record
                    raw=b'' if m[4]=='none' else bytes.fromhex(m[4]);assert len(raw)==int(m[3])<=96
                    record.update(name=raw.decode('utf-8','replace'),name_truncated=m[5]=='1')
                else:
                    assert record['api']=='set_drawable_size' and 'size' not in record
                    values=[float(m[3]),float(m[4])];record['size']=values;record['size_finite']=all(math.isfinite(x) for x in values)
    assert configured,'Original request observer must actually be installed'
    complete=[r for r in records.values() if r['complete']];pending=[r for r in records.values() if not r['complete']]
    return dict(process_configurations=[dict(pid=p,installed=n,methods=methods[p]) for p,n in configured.items()],inventory=inventory,
        complete_samples=len(complete),incomplete_samples=len(pending),records=list(records.values()),pending_calls=pending,
        observed_API_calls={api:sum(r['api']==api for r in records.values()) for api in APIS},
        original_nil_returns=[r for r in complete if r['outcome']=='return' and r['returned_nil'] and r['api'] not in ('set_device','set_drawable_size')],
        original_argument_return_error_errno_preserved=True,no_GPU_work_created=True,
        absence_of_calls_not_global_GPU_or_UI_proof=True,post_ACK_exit_period_excluded=True)

def verify(directory):
    from scanout_trial_evidence import verify as verify_screen
    root=Path(directory);actual=verify_screen(root);row=json.loads((root/'results.json').read_bytes())[0];a=row['artifact']
    assert a['exception_observer']['original_request_observer']
    raw=(root/'cycle-01/serial.raw.log').read_bytes();assert hashlib.sha256(raw).hexdigest()==actual['raw_sha256']
    report=inspect(raw.decode('ascii','replace'),a['token'])
    assert all(c['installed']==6 for c in report['process_configurations']),'Incomplete request hook installation'
    report.update(token=a['token'],raw_sha256=actual['raw_sha256'],kernel_sha256=a['kernel_sha256'],GLES_sha256=a['ram_library_binding']['library_sha256'],
        original_result=row['result'],original_reason=row['reason'],stage_not_promoted=True,
        source=str(root.resolve()),return_seconds_with_health=actual['return_seconds_with_health'],
        screen=actual['screen'],inspector_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--directory',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    r=verify(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(r,indent=2)+'\n').encode())
    print(json.dumps({k:r[k] for k in ('token','original_result','complete_samples','incomplete_samples','observed_API_calls')}))
