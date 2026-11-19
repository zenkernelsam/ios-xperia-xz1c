"""Pair real original request calls; missing draws stay missing, never repaired."""
import argparse,hashlib,json,math,re
from pathlib import Path

APIS=('function','function_constants','function_names','next_drawable','set_device','set_drawable_size')
ARGUMENTS=dict(function=3,function_constants=5,function_names=2,next_drawable=2,set_device=3,set_drawable_size=3)
EXTRA_APIS=('quad_init','quad_init_async','quad_layout','quad_display','quad_prewarm','quad_drawable_size','quad_update_size')
EXTRA_ARGUMENTS=dict(quad_init=6,quad_init_async=7,quad_layout=2,quad_display=4,quad_prewarm=3,quad_drawable_size=2,quad_update_size=2)
METHOD=re.compile(r'SWEET METAL REQUEST METHOD pid=(\d+) api=(\w+) present=([01]) owned=([01]) installed=([01]) arguments=(\d+) exact_ABI=1')
CONFIG=re.compile(r'SWEET METAL REQUEST CONFIG pid=(\d+) installed=(\d+) expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1')
BEGIN=re.compile(r'SWEET METAL REQUEST BEGIN pid=(\d+) call=(\d+) api=(\w+) receiver=(\S+) argument_nil=([01]) argument_class=(\S+) original_forwarded=1')
END=re.compile(r'SWEET METAL REQUEST END pid=(\d+) call=(\d+) api=(\w+) outcome=(return|throw) returned_nil=([01]) return_class=(\S+) error_pointer=([01]) error_code=(-?\d+) original_behavior=1')
NAME=re.compile(r'SWEET METAL REQUEST NAME pid=(\d+) call=(\d+) bytes=(\d+) hex=(\S+) truncated=([01]) original_name_argument=1')
SIZE=re.compile(r'SWEET METAL REQUEST SIZE pid=(\d+) call=(\d+) width=(\S+) height=(\S+) original_arguments=1')
ABI=re.compile(r'SWEET METAL REQUEST ABI pid=(\d+) owner=(quad|renderer) selector=(\S+) present=([01]) types_hex=(\S+) truncated=([01]) metadata_only=1')

CONFIG2=re.compile(r'SWEET METAL REQUEST CONFIG pid=(\d+) installed=(\d+) expected=13 limit=64 revision=2 original_behavior=1 no_extra_GPU_work=1')
BEGIN2=re.compile(r'SWEET METAL REQUEST BEGIN pid=(\d+) call=(\d+) api=(\w+) original_forwarded=1')
OBJECT2=re.compile(r'SWEET METAL REQUEST OBJECT pid=(\d+) call=(\d+) receiver=(\S+) argument_nil=([01]) argument_class=(\S+)')
RETURN2=re.compile(r'SWEET METAL REQUEST RETURN pid=(\d+) call=(\d+) returned_nil=([01]) return_class=(\S+)')
ERROR2=re.compile(r'SWEET METAL REQUEST ERROR pid=(\d+) call=(\d+) pointer=([01]) code=(-?\d+)')
END2=re.compile(r'SWEET METAL REQUEST END pid=(\d+) call=(\d+) api=(\w+) outcome=(return|throw) original_behavior=1')
NAME2=re.compile(r'SWEET METAL REQUEST NAME pid=(\d+) call=(\d+) bytes=(\d+) truncated=([01]) original_name_argument=1')
NAME_DATA2=re.compile(r'SWEET METAL REQUEST NAME_DATA pid=(\d+) call=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
INIT2=re.compile(r'SWEET METAL REQUEST INIT pid=(\d+) call=(\d+) options=(\d+) color_space=(-?\d+) async_nil=([01])')
ORIGIN2=re.compile(r'SWEET METAL REQUEST ORIGIN pid=(\d+) call=(\d+) x=(\S+) y=(\S+)')
BOOL2=re.compile(r'SWEET METAL REQUEST BOOL pid=(\d+) call=(\d+) value=([01])')
FLAG2=re.compile(r'SWEET METAL REQUEST FLAG pid=(\d+) call=(\d+) value=([01])')
ABI2=re.compile(r'SWEET METAL REQUEST ABI pid=(\d+) owner=(quad|renderer) index=(\d+) present=([01]) name_bytes=(\d+) type_bytes=(\d+) truncated=([01])')
ABI_DATA2=re.compile(r'SWEET METAL REQUEST ABI_DATA pid=(\d+) owner=(quad|renderer) index=(\d+) part=(types|name) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
ABI_END2=re.compile(r'SWEET METAL REQUEST ABI_END pid=(\d+) owner=(quad|renderer) index=(\d+)')

def inspect_short(scope):
    apis=APIS+EXTRA_APIS;arguments={**ARGUMENTS,**EXTRA_ARGUMENTS}
    methods={};configured={};records={};abi={};inventory=[]
    patterns=(METHOD,CONFIG2,BEGIN2,OBJECT2,RETURN2,ERROR2,END2,NAME2,NAME_DATA2,SIZE,INIT2,ORIGIN2,BOOL2,FLAG2,ABI2,ABI_DATA2,ABI_END2)
    matches=sorted((m.start(),i,m) for i,p in enumerate(patterns) for m in p.finditer(scope))
    assert len(matches)==scope.count('SWEET METAL REQUEST '),'Truncated short request record'
    for _,kind,m in matches:
        pid=int(m[1]);assert pid>0
        if kind==0:
            api=m[2];entry=methods.setdefault(pid,{})
            assert api in apis and api not in entry and pid not in configured
            entry[api]=dict(present=m[3]=='1',owned=m[4]=='1',installed=m[5]=='1',arguments=int(m[6]))
            assert not entry[api]['installed'] or (entry[api]['present'] and entry[api]['owned'] and entry[api]['arguments']==arguments[api])
        elif kind==1:
            assert pid in methods and set(methods[pid])==set(apis) and pid not in configured
            assert not any(k[0]==pid for k in abi)
            configured[pid]=int(m[2]);assert configured[pid]==sum(r['installed'] for r in methods[pid].values())
            assert sum(i['pid']==pid for i in inventory)==10
        elif kind>=14:
            assert pid in methods and pid not in configured;key=(pid,m[2],int(m[3]))
            if kind==14:
                assert key not in abi and 0<=key[2]<(6 if key[1]=='quad' else 4)
                abi[key]=dict(present=m[4]=='1',name_bytes=int(m[5]),types_bytes=int(m[6]),truncated=m[7]=='1',name=bytearray(),types=bytearray())
                assert 0<abi[key]['name_bytes']<=128 and 0<=abi[key]['types_bytes']<=144
                assert bool(abi[key]['types_bytes'])==abi[key]['present']
            elif kind==15:
                assert key in abi;item=abi[key];part=m[4];data=bytes.fromhex(m[7])
                assert int(m[5])==len(item[part]) and len(data)==int(m[6]) and 0<len(data)<=24
                assert len(item[part])+len(data)<=item[part+'_bytes'];item[part].extend(data)
            else:
                assert key in abi;item=abi.pop(key)
                assert len(item['name'])==item['name_bytes'] and len(item['types'])==item['types_bytes']
                inventory.append(dict(pid=pid,owner=key[1],index=key[2],selector=item['name'].decode('ascii'),types=item['types'].decode('ascii'),present=item['present'],truncated=item['truncated']))
        else:
            assert pid in configured;key=(pid,int(m[2]));assert 1<=key[1]<=64
            if kind==2:
                assert key not in records and m[3] in apis and methods[pid][m[3]]['installed']
                records[key]=dict(pid=pid,call=key[1],api=m[3],complete=False)
                continue
            assert key in records;record=records[key];assert not record['complete']
            if kind==3:
                assert 'receiver' not in record and (m[4]=='1')==(m[5]=='nil')
                record.update(receiver=m[3],argument_nil=m[4]=='1',argument_class=m[5])
            elif kind==4:
                assert 'returned_nil' not in record and (m[3]=='1')==(m[4]=='nil')
                record.update(returned_nil=m[3]=='1',return_class=m[4])
            elif kind==5:
                assert 'error_pointer' not in record and (not int(m[4]) or m[3]=='1')
                record.update(error_pointer=m[3]=='1',error_code=int(m[4]))
            elif kind==6:
                assert m[3]==record['api'] and all(k in record for k in ('receiver','returned_nil','error_pointer'))
                if record['api'] in ('function','function_constants'):
                    assert len(record['name_raw'])==record.pop('name_bytes');record['name']=bytes(record.pop('name_raw')).decode('utf-8','replace')
                if record['api'] in ('set_drawable_size','quad_drawable_size','quad_init','quad_init_async'):assert 'size' in record
                if record['api'] in ('quad_init','quad_init_async'):assert 'origin' in record and 'options' in record
                if record['api'] in ('quad_display','quad_prewarm'):assert 'boolean_result' in record
                if record['api']=='quad_display':assert 'flag' in record
                record.update(complete=True,outcome=m[4])
            elif kind==7:
                assert record['api'] in ('function','function_constants') and 'name_raw' not in record
                record.update(name_bytes=int(m[3]),name_truncated=m[4]=='1',name_raw=bytearray());assert 0<=int(m[3])<=96
            elif kind==8:
                assert 'name_raw' in record;data=bytes.fromhex(m[5]);assert len(data)==int(m[4]) and 0<len(data)<=24
                assert int(m[3])==len(record['name_raw']) and int(m[3])+len(data)<=record['name_bytes'];record['name_raw'].extend(data)
            elif kind==9:
                assert 'size' not in record;record['size']=[float(m[3]),float(m[4])];record['size_finite']=all(math.isfinite(x) for x in record['size'])
            elif kind==10:
                assert record['api'] in ('quad_init','quad_init_async') and 'options' not in record
                record.update(options=int(m[3]),color_space=int(m[4]),async_nil=m[5]=='1')
                assert 0<=record['options']<1<<64 and -(1<<63)<=record['color_space']<1<<63
            elif kind==11:
                assert 'origin' not in record and record['api'] in ('quad_init','quad_init_async');record['origin']=[float(m[3]),float(m[4])]
            elif kind==12:
                assert 'boolean_result' not in record and record['api'] in ('quad_display','quad_prewarm');record['boolean_result']=m[3]=='1'
            else:
                assert 'flag' not in record and record['api']=='quad_display';record['flag']=m[3]=='1'
    assert configured and not abi
    pending=[r for r in records.values() if not r['complete']];complete=[r for r in records.values() if r['complete']]
    for r in pending:
        if 'name_raw' in r:r['name_partial_hex']=bytes(r.pop('name_raw')).hex()
    voids=('set_device','set_drawable_size','quad_layout','quad_update_size','quad_display','quad_prewarm','quad_drawable_size')
    return dict(revision=2,process_configurations=[dict(pid=p,installed=n,methods=methods[p]) for p,n in configured.items()],inventory=inventory,
        complete_samples=len(complete),incomplete_samples=len(pending),records=list(records.values()),pending_calls=pending,
        observed_API_calls={api:sum(r['api']==api for r in records.values()) for api in apis},
        original_nil_returns=[r for r in complete if r['outcome']=='return' and r['returned_nil'] and r['api'] not in voids],
        original_argument_return_error_errno_preserved=True,no_GPU_work_created=True,
        absence_of_calls_not_global_GPU_or_UI_proof=True,post_ACK_exit_period_excluded=True)

def inspect(text,token):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1;scope=text[text.index(live):text.index(ack)]
    if 'expected=13 limit=64 revision=2' in scope:return inspect_short(scope)
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
    assert all(c['installed']==a['exception_observer']['request_methods'] for c in report['process_configurations']),'Incomplete request hook installation'
    report.update(token=a['token'],raw_sha256=actual['raw_sha256'],kernel_sha256=a['kernel_sha256'],GLES_sha256=a['ram_library_binding']['library_sha256'],
        original_result=row['result'],original_reason=row['reason'],stage_not_promoted=True,
        source=str(root.resolve()),return_seconds_with_health=actual['return_seconds_with_health'],
        screen=actual['screen'],inspector_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--directory',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    r=verify(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(r,indent=2)+'\n').encode())
    print(json.dumps({k:r[k] for k in ('token','original_result','complete_samples','incomplete_samples','observed_API_calls')}))
