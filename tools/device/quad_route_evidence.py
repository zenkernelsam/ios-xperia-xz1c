"""Verify bounded original scene/queue requests without inventing missing draws."""
import argparse
import hashlib
import json
import math
import re
import struct
from pathlib import Path

APIS = ('manager_device', 'manager_queue', 'device_queue', 'add_quad', 'add_quads',
        'window', 'paused', 'disable', 'enable', 'sync', 'tick', 'scene')
ARGUMENTS = dict(zip(APIS, (2, 2, 2, 3, 3, 2, 3, 3, 3, 6, 3, 3)))
TYPES = dict(zip(APIS, ('@16@0:8', '@16@0:8', '@16@0:8', 'v24@0:8@16', 'v24@0:8@16',
                       'v16@0:8', 'v20@0:8B16', 'v24@0:8@16', 'v24@0:8@16',
                       'B44@0:8d16@24B32@?36', 'v24@0:8@16', 'v24@0:8@16')))
EXTRA_APIS = ('preview', 'snapshot', 'low_light', 'backlight', 'linear_backlight', 'unlock',
              'min_size', 'desired_size', 'start', 'stop', 'update', 'snapshot_image', 'env_init', 'env_update')
EXTRA_ARGUMENTS = dict(zip(EXTRA_APIS, (2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 5)))
EXTRA_TYPES = dict(zip(EXTRA_APIS, ('B16@0:8', 'B16@0:8', 'B16@0:8', 'd16@0:8', 'd16@0:8', 'd16@0:8',
                                   '{CGSize=dd}16@0:8', '{CGSize=dd}16@0:8', 'v16@0:8', 'v16@0:8', 'v16@0:8', '@16@0:8',
                                   'v32@0:8@16@24', 'v40@0:8@16@24@32')))
METHOD = re.compile(r'SWEET QUAD ROUTE METHOD pid=(\d+) api=(\w+) present=([01]) owned=([01]) installed=([01]) arguments=(\d+) owner_checked=([01]) meta=([01])')
ABI = re.compile(r'SWEET QUAD ROUTE ABI pid=(\d+) api=(\w+) bytes=(\d+) truncated=([01])')
ABI_DATA = re.compile(r'SWEET QUAD ROUTE ABI_DATA pid=(\d+) api=(\w+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
ABI_END = re.compile(r'SWEET QUAD ROUTE ABI_END pid=(\d+) api=(\w+)')
CONFIG = re.compile(r'SWEET QUAD ROUTE CONFIG pid=(\d+) installed=(\d+) expected=12 per_api=8 max=96 revision=1 original_behavior=1 no_extra_GPU_work=1')
BEGIN = re.compile(r'SWEET QUAD ROUTE BEGIN pid=(\d+) call=(\d+) api=(\w+) original_forwarded=1')
OBJECT = re.compile(r'SWEET QUAD ROUTE OBJECT pid=(\d+) call=(\d+) receiver=(\S+) argument_nil=([01]) argument_class=(\S+)')
RETURN = re.compile(r'SWEET QUAD ROUTE RETURN pid=(\d+) call=(\d+) returned_nil=([01]) return_class=(\S+)')
END = re.compile(r'SWEET QUAD ROUTE END pid=(\d+) call=(\d+) api=(\w+) outcome=(return|throw) original_behavior=1')
FLAG = re.compile(r'SWEET QUAD ROUTE FLAG pid=(\d+) call=(\d+) value=([01])')
SYNC = re.compile(r'SWEET QUAD ROUTE SYNC pid=(\d+) call=(\d+) time=(\S+) check=([01]) completion_nil=([01])')
BOOL = re.compile(r'SWEET QUAD ROUTE BOOL pid=(\d+) call=(\d+) value=([01])')
REASON = re.compile(r'SWEET QUAD ROUTE REASON pid=(\d+) call=(\d+) string=([01]) bytes=(\d+) truncated=([01]) getter_error=([01])')
REASON_DATA = re.compile(r'SWEET QUAD ROUTE REASON_DATA pid=(\d+) call=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)')
LIMIT = re.compile(r'SWEET QUAD ROUTE LIMIT pid=(\d+) api=(\w+) per_api=8 original_forwarded=1')
CONFIG2 = re.compile(r'SWEET QUAD ROUTE CONFIG pid=(\d+) installed=(\d+) expected=26 per_api=8 max=208 revision=2 original_behavior=1 no_extra_GPU_work=1')
IDENTITY = re.compile(r'SWEET QUAD ROUTE ID pid=(\d+) call=(\d+) receiver=([0-9a-f]{16}) argument=([0-9a-f]{16})')
DOUBLE = re.compile(r'SWEET QUAD ROUTE DOUBLE pid=(\d+) call=(\d+) bits=([0-9a-f]{16})')
SIZE = re.compile(r'SWEET QUAD ROUTE SIZE pid=(\d+) call=(\d+) width_bits=([0-9a-f]{16}) height_bits=([0-9a-f]{16})')
ENV = re.compile(r'SWEET QUAD ROUTE ENV pid=(\d+) call=(\d+) renderer=([0-9a-f]{16}) transition=([0-9a-f]{16})')


def inspect(text, token):
    live, ack = 'SWEET LOOP LIVE proto=1 token='+token, 'SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live) == text.count(ack) == 1
    scope = text[text.index(live):text.index(ack)]
    revision = 2 if 'expected=26 per_api=8 max=208 revision=2' in scope else 1
    apis = APIS+EXTRA_APIS if revision==2 else APIS
    arguments = {**ARGUMENTS, **EXTRA_ARGUMENTS}; types = {**TYPES, **EXTRA_TYPES}; maximum = len(apis)*8
    patterns = (METHOD, ABI, ABI_DATA, ABI_END, CONFIG2 if revision==2 else CONFIG, BEGIN, OBJECT, RETURN, END, FLAG, SYNC, BOOL, REASON, REASON_DATA, LIMIT)
    if revision==2: patterns += (IDENTITY, DOUBLE, SIZE, ENV)
    matches = sorted((m.start(), i, m) for i, p in enumerate(patterns) for m in p.finditer(scope))
    assert len(matches) == scope.count('SWEET QUAD ROUTE '), 'Truncated or unknown original route record'
    methods, inventory, configured, records, limits = {}, {}, {}, {}, set()
    for _, kind, m in matches:
        pid = int(m[1]); assert pid > 0
        if kind < 5:
            assert pid not in configured
            if kind == 0:
                api = m[2]; assert api in apis and (pid, api) not in methods
                entry = dict(present=m[3]=='1', owned=m[4]=='1', installed=m[5]=='1', arguments=int(m[6]),
                             owner_checked=m[7]=='1', metaclass=m[8]=='1')
                assert entry['metaclass'] == (api in APIS[:2])
                if entry['installed']:
                    assert entry['present'] and entry['owned'] and entry['owner_checked'] and entry['arguments']==arguments[api]
                methods[pid, api] = entry
            elif kind in (1, 2, 3):
                api = m[2]; key = (pid, api); assert key in methods
                if kind == 1:
                    assert key not in inventory
                    inventory[key] = dict(bytes=int(m[3]), truncated=m[4]=='1', raw=bytearray(), complete=False)
                    assert 0 <= int(m[3]) <= 144 and bool(int(m[3])) == methods[key]['present']
                elif kind == 2:
                    assert key in inventory; entry = inventory[key]; data = bytes.fromhex(m[5])
                    assert not entry['complete'] and int(m[3]) == len(entry['raw']) and 0 < len(data) == int(m[4]) <= 24
                    assert len(entry['raw'])+len(data) <= entry['bytes']; entry['raw'].extend(data)
                else:
                    entry = inventory[key]; assert not entry['complete'] and len(entry['raw'])==entry['bytes']
                    entry['types'] = bytes(entry.pop('raw')).decode('ascii'); entry['complete'] = True
                    if methods[key]['installed']: assert not entry['truncated'] and entry['types']==types[api]
            else:
                assert set(api for p, api in methods if p==pid) == set(apis)
                assert set(api for p, api in inventory if p==pid) == set(apis)
                assert all(v['complete'] for (p, _), v in inventory.items() if p==pid)
                count = int(m[2]); assert count == sum(v['installed'] for (p, _), v in methods.items() if p==pid)
                configured[pid] = count
            continue
        assert pid in configured
        if kind == 14:
            api = m[2]; key = (pid, api)
            assert api in apis and key not in limits and sum(r['api']==api and r['pid']==pid for r in records.values())==8
            limits.add(key); continue
        key = (pid, int(m[2])); assert 1 <= key[1] <= maximum
        if kind == 5:
            api = m[3]; assert key not in records and api in apis and methods[pid, api]['installed']
            assert sum(r['pid']==pid and r['api']==api for r in records.values()) < 8
            assert key[1] == 1+sum(r['pid']==pid for r in records.values())
            records[key] = dict(pid=pid, call=key[1], api=api, complete=False)
            continue
        assert key in records; record = records[key]; assert not record['complete']
        if kind == 6:
            assert 'receiver' not in record and (m[4]=='1')==(m[5]=='nil')
            record.update(receiver=m[3], argument_nil=m[4]=='1', argument_class=m[5])
        elif kind == 7:
            assert 'returned_nil' not in record and (m[3]=='1')==(m[4]=='nil')
            record.update(returned_nil=m[3]=='1', return_class=m[4])
        elif kind == 8:
            assert m[3]==record['api'] and all(k in record for k in ('receiver', 'returned_nil'))
            if revision==2: assert 'receiver_identity' in record
            if record['api']=='paused': assert 'paused' in record
            if record['api']=='sync': assert 'time' in record and 'boolean_result' in record
            if record['api'] in ('preview', 'snapshot', 'low_light'): assert 'boolean_result' in record
            if record['api'] in ('backlight', 'linear_backlight', 'unlock'): assert 'double_bits' in record
            if record['api'] in ('min_size', 'desired_size'): assert 'size_bits' in record
            if record['api'] in ('env_init', 'env_update'): assert 'renderer_identity' in record
            if record['api'] in ('disable', 'enable'):
                assert 'reason_raw' in record and len(record['reason_raw'])==record.pop('reason_bytes')
                record['reason'] = bytes(record.pop('reason_raw')).decode('utf-8', 'replace')
            record.update(complete=True, outcome=m[4])
        elif kind == 9:
            assert record['api']=='paused' and 'paused' not in record; record['paused']=m[3]=='1'
        elif kind == 10:
            assert record['api']=='sync' and 'time' not in record
            record.update(time=float(m[3]), check_for_drawable=m[4]=='1', completion_nil=m[5]=='1')
            record['time_finite'] = math.isfinite(record['time'])
        elif kind == 11:
            assert record['api'] in ('sync', 'preview', 'snapshot', 'low_light') and 'boolean_result' not in record; record['boolean_result']=m[3]=='1'
        elif kind == 12:
            assert record['api'] in ('disable', 'enable') and 'reason_raw' not in record
            size = int(m[4]); assert 0 <= size <= 96 and (m[3]=='1' or not size)
            assert m[6]!='1' or size==0
            record.update(reason_string=m[3]=='1', reason_bytes=size, reason_truncated=m[5]=='1', reason_getter_error=m[6]=='1', reason_raw=bytearray())
        elif kind==13:
            assert 'reason_raw' in record; data = bytes.fromhex(m[5])
            assert 0 < len(data)==int(m[4]) <= 24 and int(m[3])==len(record['reason_raw'])
            assert len(record['reason_raw'])+len(data) <= record['reason_bytes']; record['reason_raw'].extend(data)
        elif kind==15:
            assert 'receiver_identity' not in record and 'receiver' in record
            assert (int(m[3],16)==0)==(record['receiver']=='nil') and (int(m[4],16)==0)==record['argument_nil']
            record.update(receiver_identity=m[3], argument_identity=m[4])
        elif kind==16:
            assert record['api'] in ('backlight', 'linear_backlight', 'unlock') and 'double_bits' not in record
            value = struct.unpack('<d', int(m[3],16).to_bytes(8,'little'))[0]
            record.update(double_bits=m[3], double_value=value if math.isfinite(value) else None, double_finite=math.isfinite(value))
        elif kind==17:
            assert record['api'] in ('min_size', 'desired_size') and 'size_bits' not in record
            values = [struct.unpack('<d',int(m[k],16).to_bytes(8,'little'))[0] for k in (3,4)]
            record.update(size_bits=[m[3],m[4]], size_values=[v if math.isfinite(v) else None for v in values], size_finite=all(math.isfinite(v) for v in values))
        else:
            assert kind==18 and record['api'] in ('env_init','env_update') and 'renderer_identity' not in record
            assert record['api']!='env_init' or int(m[4],16)==0
            record.update(renderer_identity=m[3], transition_identity=m[4])
    assert configured
    complete = [r for r in records.values() if r['complete']]; pending = [r for r in records.values() if not r['complete']]
    for r in pending:
        if 'reason_raw' in r: r['reason_partial_hex'] = bytes(r.pop('reason_raw')).hex()
    return dict(revision=revision, process_configurations=[dict(pid=pid, installed=count, methods={api:methods[pid, api] for api in apis}) for pid, count in configured.items()],
                inventory=[dict(pid=p, api=api, **value) for (p, api), value in inventory.items()],
                complete_samples=len(complete), incomplete_samples=len(pending), records=list(records.values()), pending_calls=pending,
                observed_API_calls={api:sum(r['api']==api for r in records.values()) for api in apis},
                sampling_limits=[dict(pid=p, api=api) for p, api in sorted(limits)],
                original_nil_resource_returns=[r for r in complete if r['api'] in APIS[:3] and r['outcome']=='return' and r['returned_nil']],
                per_API_record_counts_are_not_total_calls=True, original_arguments_results_errno_throws_preserved=True,
                no_additional_GPU_or_scene_calls=True, post_ACK_exit_period_excluded=True,
                absent_calls_are_not_global_GPU_or_UI_proof=True)


def verify(directory):
    from scanout_trial_evidence import verify as verify_screen
    root = Path(directory); screen = verify_screen(root); row = json.loads((root/'results.json').read_bytes())[0]
    artifact = row['artifact']; observer = artifact['exception_observer']
    assert observer['quad_route_observer'] and observer['quad_route_revision'] in (1,2)
    raw = (root/'cycle-01/serial.raw.log').read_bytes(); assert hashlib.sha256(raw).hexdigest()==screen['raw_sha256']
    result = inspect(raw.decode('ascii', 'replace'), artifact['token'])
    assert result['revision']==observer['quad_route_revision']
    assert all(p['installed']==observer['quad_route_methods']==(12 if result['revision']==1 else 26) for p in result['process_configurations'])
    result.update(token=artifact['token'], kernel_sha256=artifact['kernel_sha256'], GLES_sha256=artifact['ram_library_binding']['library_sha256'],
                  raw_sha256=screen['raw_sha256'], original_result=row['result'], original_reason=row['reason'], stage_not_promoted=True,
                  source=str(root.resolve()), return_seconds_with_health=screen['return_seconds_with_health'], screen=screen['screen'],
                  inspector_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    return result


if __name__=='__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--directory', required=True); parser.add_argument('--output', required=True)
    args = parser.parse_args(); result = verify(args.directory); out = Path(args.output)
    with out.open('xb') as f: f.write((json.dumps(result, indent=2)+'\n').encode())
    print(json.dumps({k:result[k] for k in ('token', 'original_result', 'complete_samples', 'incomplete_samples', 'observed_API_calls')}))
