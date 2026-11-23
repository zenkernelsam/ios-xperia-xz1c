"""Verify current-scope before/after cache metadata without claiming a GPU import."""
import argparse
import hashlib
import json
import re
from pathlib import Path

PATTERNS = (
    re.compile(r'SWEET PCINV BEGIN pid=(\d+) phase=([BA]) revision=1 max_files=64 max_bytes=33554432 read_only=1 no_compile=1'),
    re.compile(r'SWEET PCINV ID pid=(\d+) phase=([BA]) value=([0-9a-f]{64})'),
    re.compile(r'SWEET PCINV ITEM pid=(\d+) phase=([BA]) index=(\d+) kind=([RB]) vs=(\d+) fs=(\d+) bytes=(\d+) format=(\d+)'),
    re.compile(r'SWEET PCINV (KEY|SHA|CFG|VS|FS) pid=(\d+) phase=([BA]) index=(\d+) value=([0-9a-f]{64})'),
    re.compile(r'SWEET PCINV ENDITEM pid=(\d+) phase=([BA]) index=(\d+) record_verified=1 import_performed=0'),
    re.compile(r'SWEET PCINV END pid=(\d+) phase=([BA]) result=(-?\d+) files=(\d+) records=(\d+) requests=(\d+) bins=(\d+) present=(\d+) missing=(\d+)'),
    re.compile(r'SWEET PCINV TOTAL pid=(\d+) phase=([BA]) bytes=(\d+) pending=(\d+) orphans=(\d+) read_only=1 import_performed=0'),
    re.compile(r'SWEET PCINV ACTION pid=(\d+) action=(prime|skip) missing=(\d+) max_entries=8 actual_files_checked=1'),
    re.compile(r'SWEET PCINV PRIME pid=(\d+) result=(-?\d+) compiled=(\d+) failed=(\d+) expected=(\d+)'),
    re.compile(r'SWEET PCINV COMPLETE pid=(\d+) result=(-?\d+) verified_before_after=([01])'),
)


def inspect_scope(scope, spec, require_runtime_prime=True):
    matches = sorted((m.start(), i, m) for i, p in enumerate(PATTERNS) for m in p.finditer(scope))
    if len(matches) != scope.count('SWEET PCINV '):
        raise ValueError('Unknown or truncated original cache inventory record')
    snapshots, records, pid = {}, {}, None
    active = None
    action = prime = complete = None
    action_at = prime_at = complete_at = None
    for at, kind, m in matches:
        current_pid = int(m[2] if kind == 3 else m[1])
        if current_pid <= 0 or pid is not None and current_pid != pid:
            raise ValueError('Inventory process changed')
        pid = current_pid
        if kind == 0:
            phase = m[2]
            if active is not None or phase in snapshots or (not snapshots and phase != 'B'):
                raise ValueError('Repeated, missing or overlapping inventory phase')
            if phase == 'A' and (not spec['conditional_prime'] or action is None):
                raise ValueError('After inventory without a checked conditional action')
            snapshots[phase] = {'phase': phase, 'begin_at': at, 'identity': None, 'end': None, 'total': None}
            active = phase
            continue
        if kind in (1, 2, 3, 4, 5, 6):
            phase = m[3] if kind == 3 else m[2]
            if phase not in snapshots or active != phase:
                raise ValueError('Inventory data outside its phase')
            snap = snapshots[phase]
            if kind == 1:
                if snap['identity'] is not None or m[3] != spec['context_identity']:
                    raise ValueError('Current inventory context identity differs')
                snap['identity'] = m[3]
                continue
            if snap['identity'] is None:
                raise ValueError('Inventory item before identity')
            if kind in (2, 3, 4):
                index = int(m[4] if kind == 3 else m[3])
                key = phase, index
                if snap['end'] is not None:
                    raise ValueError('Inventory item after phase end')
                if kind == 2:
                    if key in records or index != sum(p == phase for p, _ in records) or index >= spec['max_files']:
                        raise ValueError('Repeated or missing cache item')
                    vs, fs, count, fmt = map(int, m.groups()[4:])
                    if not 0 < vs <= 1048576 or not 0 < fs <= 1048576 or not 136 < count <= 136+spec['max_payload']:
                        raise ValueError('Cache record outside actual bounds')
                    if m[4] == 'R' and (fmt or count != 136+316+vs+fs) or m[4] == 'B' and not fmt:
                        raise ValueError('Cache request/binary structure differs')
                    records[key] = dict(index=index, kind=m[4], vertex_bytes=vs,
                                        fragment_bytes=fs, bytes=count, format=fmt, complete=False)
                elif key not in records or records[key]['complete']:
                    raise ValueError('Cache item fields before header or after end')
                elif kind == 3:
                    label = m[1].lower()
                    if label in records[key] or records[key]['kind'] == 'B' and label not in ('key', 'sha'):
                        raise ValueError('Repeated or foreign cache digest field')
                    records[key][label] = m[5]
                else:
                    required = {'key', 'sha'} | ({'cfg', 'vs', 'fs'} if records[key]['kind'] == 'R' else set())
                    if not required <= records[key].keys():
                        raise ValueError('Missing complete cache digest capture')
                    records[key]['complete'] = True
                continue
            if kind == 5:
                if snap['end'] is not None or int(m[3]) != 0 or any(not r['complete'] for (p, _), r in records.items() if p == phase):
                    raise ValueError('Original inventory failed or has incomplete items')
                values = list(map(int, m.groups()[3:]))
                snap['end'] = dict(zip(('files', 'records', 'requests', 'bins', 'present', 'missing'), values))
                snap['end_at'] = at
            else:
                if snap['end'] is None or snap['total'] is not None:
                    raise ValueError('Inventory total before end or repeated')
                snap['total'] = dict(bytes=int(m[3]), pending=int(m[4]), orphans=int(m[5]))
                snap['total_at'] = at
                active = None
            continue
        if active is not None or 'B' not in snapshots or snapshots['B']['total'] is None:
            raise ValueError('Action before verified inventory')
        if kind == 7:
            if action is not None or 'A' in snapshots or not spec['conditional_prime']:
                raise ValueError('Repeated or unexpected cache action')
            action = dict(action=m[2], missing=int(m[3]))
            action_at = at
        elif kind == 8:
            if action is None or action['action'] != 'prime' or prime is not None or 'A' in snapshots:
                raise ValueError('Unexpected or repeated actual primer')
            prime = dict(result=int(m[2]), compiled=int(m[3]), failed=int(m[4]), expected=int(m[5]))
            prime_at = at
        else:
            if complete is not None or int(m[2]) or m[3] != '1':
                raise ValueError('Missing or failed inventory completion')
            complete, complete_at = True, at
    phases = {'B', 'A'} if spec['conditional_prime'] else {'B'}
    if set(snapshots) != phases or active is not None or not complete:
        raise ValueError('Missing original cache phase/completion')
    for phase, snap in snapshots.items():
        rows = [r for (p, _), r in records.items() if p == phase]
        req = {r['key']: r for r in rows if r['kind'] == 'R'}
        bins = {r['key']: r for r in rows if r['kind'] == 'B'}
        if len(req)+len(bins) != len(rows):
            raise ValueError('Repeated immutable cache file key')
        for key in req.keys() & bins.keys():
            if (req[key]['vertex_bytes'], req[key]['fragment_bytes']) != (bins[key]['vertex_bytes'], bins[key]['fragment_bytes']):
                raise ValueError('Paired binary source lengths differ')
        expected = dict(files=len(rows)+snap['total']['pending'], records=len(rows),
                        requests=len(req), bins=len(bins), present=len(req.keys() & bins.keys()),
                        missing=len(req.keys()-bins.keys()))
        if snap['end'] != expected or snap['total']['orphans'] != len(bins.keys()-req.keys()):
            raise ValueError('Original cache summary disagrees with its records')
        if not 0 <= snap['end']['files'] <= spec['max_files'] or not sum(r['bytes'] for r in rows) <= snap['total']['bytes'] <= spec['max_bytes']:
            raise ValueError('Inventory count/byte bound differs')
        if not snap['total']['pending'] and snap['total']['bytes'] != sum(r['bytes'] for r in rows):
            raise ValueError('Unknown published cache bytes')
        if complete_at <= snap['total_at']:
            raise ValueError('Completion before complete metadata')
        snap.update(records=rows, request_keys=sorted(req), binary_keys=sorted(bins))
    if spec['conditional_prime']:
        before, after = snapshots['B'], snapshots['A']
        missing = before['end']['missing']
        if action is None or action['missing'] != missing or missing > spec['max_prime_entries']:
            raise ValueError('Action differs from actual missing request files')
        if action['action'] != ('prime' if missing else 'skip'):
            raise ValueError('Empty primer or missing primer dispatch')
        if not before['total_at'] < action_at < after['begin_at']:
            raise ValueError('Conditional cache action outside original snapshots')
        before_req = {r['key']: r for r in before['records'] if r['kind'] == 'R'}
        after_req = {r['key']: r for r in after['records'] if r['kind'] == 'R'}
        before_bin = {r['key']: r for r in before['records'] if r['kind'] == 'B'}
        after_bin = {r['key']: r for r in after['records'] if r['kind'] == 'B'}
        if before_req != after_req or any(after_bin.get(k) != r for k, r in before_bin.items()):
            # Indices change when newly added binaries sort before old requests.
            def stable(r): return {k:v for k,v in r.items() if k != 'index'}
            if {k:stable(r) for k,r in before_req.items()} != {k:stable(r) for k,r in after_req.items()} or any(k not in after_bin or stable(after_bin[k]) != stable(r) for k,r in before_bin.items()):
                raise ValueError('Existing immutable request/binary bytes changed')
        if set(after_bin) != set(before_bin) | set(before_req) or after['end']['missing'] or before['total']['pending'] != after['total']['pending']:
            raise ValueError('Missing or unexpected post-prime cache files')
        if missing:
            expected = dict(result=0, compiled=missing, failed=0, expected=missing)
            if prime != expected or not action_at < prime_at < after['begin_at']:
                raise ValueError('Actual primer count/completion differs')
        elif prime is not None:
            raise ValueError('Primer ran with no missing request files')
        block = scope[action_at:after['begin_at']]
        starts = re.findall(r'SWEET PROGRAM CACHE prime_begin vertex_bytes=(\d+) fragment_bytes=(\d+) source_verified=1', block)
        if require_runtime_prime:
            finals = re.findall(r'SWEET PROGRAM CACHE PRIME END result=0 compiled=(\d+) failed=0 max_entries=8 original_requests=1', block)
            entries = re.findall(r'SWEET PROGRAM CACHE prime_entry result=0 cache_state=2 reason=', block)
            wanted = sorted((before_req[k]['vertex_bytes'], before_req[k]['fragment_bytes']) for k in before_req.keys()-before_bin.keys())
            if missing and (finals != [str(missing)] or len(entries) != missing or sorted((int(v),int(f)) for v,f in starts) != wanted):
                raise ValueError('Actual current-source primer events differ from missing files')
            if not missing and (starts or finals or entries):
                raise ValueError('Empty cache primer actually ran')
    return dict(pid=pid, snapshots=snapshots, action=action, prime=prime,
                actual_file_records_only=True, inventory_did_not_import_binary_or_draw=True,
                existing_files_preserved=True, missing_bins_compiled=bool(action and action['action']=='prime'),
                cache_metadata_not_full_UI_or_GPU_import_proof=True)


def inspect(text, token, spec):
    live, ack = 'SWEET LOOP LIVE proto=1 token='+token, 'SWEET LOOP ACK FASTBOOT token='+token
    if text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
        raise ValueError('Need one current LIVE/ACK interval')
    scope = text[text.index(live):text.index(ack)]
    first = scope.find('SWEET PCINV BEGIN ')
    if first < 0:
        raise ValueError('Original cache inventory missing')
    native = scope.find('SWEET PRESENT PROBE END result=0 phases=5')
    png = scope.find('SWEET CIF10 REFERENCE END result=0 cases=6')
    if not 0 <= native < first or not 0 <= png < first:
        raise ValueError('Inventory must follow complete native21 and PNG6')
    result = inspect_scope(scope, spec)
    result.update(token=token, post_ACK_excluded=True, native21_PNG6_before_inventory=True)
    return result


def verify(directory):
    from scanout_trial_evidence import verify as verify_screen
    root=Path(directory);screen=verify_screen(root);row=json.loads((root/'results.json').read_bytes())[0]
    spec=row['artifact']['exception_observer']['program_cache_inventory']
    raw=(root/'cycle-01/serial.raw.log').read_bytes()
    if hashlib.sha256(raw).hexdigest()!=screen['raw_sha256']:
        raise ValueError('Current raw binding differs')
    result=inspect(raw.decode('ascii','replace'),row['artifact']['token'],spec)
    result.update(source=str(root.resolve()),raw_sha256=screen['raw_sha256'],
                  original_result=row['result'],original_reason=row['reason'],stage_not_promoted=True)
    return result


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory',required=True);parser.add_argument('--output',required=True)
    args=parser.parse_args();result=verify(args.directory)
    with Path(args.output).open('x',encoding='utf-8',newline='\n') as file:
        json.dump(result,file,indent=2);file.write('\n')
    print(json.dumps({'token':result['token'],'original_result':result['original_result'],
                      'before':result['snapshots']['B']['end'],'action':result['action']}))
