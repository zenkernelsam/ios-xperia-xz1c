"""Inspect original snapshot timing without treating a timeout as UI success."""
import re

APIS = ('png', 'add_image', 'finalize', 'colorsync')
METHOD_APIS = ('pipeline', 'library_url', 'library_bundle', 'snapshot', 'wait', 'execute', 'read', 'read_full')
METHOD_ARGUMENTS = dict(pipeline=4, library_url=4, library_bundle=4, snapshot=5, wait=2, execute=2, read=6, read_full=8)
BEGIN = re.compile(r'SWEET POSTDRAW BEGIN pid=(\d+) tid=(\d+) call=(\d+) root=(\d+) depth=(\d+) api=(\w+) at_ns=(\d+) original_forwarded=1')
END = re.compile(r'SWEET POSTDRAW END pid=(\d+) tid=(\d+) call=(\d+) api=(\w+) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) outcome=(\w+) bytes=(\d+) returned_nil=([01]) original_behavior=1')
IMAGE = re.compile(r'SWEET POSTDRAW IMAGE pid=(\d+) call=(\d+) width=(\d+) height=(\d+) bpp=(\d+) pitch=(\d+) color_model=(-?\d+) properties_nil=([01]) pixels_read=0')
ORIGINAL = re.compile(r'SWEET POSTDRAW ORIGINAL pid=(\d+) api=(\w+) image=(\S+) offset=(0x[0-9a-f]+) resolved=([01])')
INSTALLED = 'SWEET POSTDRAW observer installed=1 interpose_pairs=4 limit=64 metadata_only=1 original_arguments_results=1'
METHOD_INIT = re.compile(r'SWEET SNAPSHOT INIT pid=(\d+) at_ns=(\d+) expected=8 metadata_only=1')
METHOD = re.compile(r'SWEET SNAPSHOT METHOD pid=(\d+) api=(\w+) present=([01]) owned=([01]) installed=([01]) arguments=(\d+) original_forwarded=1')
METHOD_CONFIG = re.compile(r'SWEET SNAPSHOT observer pid=(\d+) installed=(\d+) expected=8 limit=64 metadata_only=1 original_arguments_results=1')
OBJECT = re.compile(r'SWEET SNAPSHOT OBJECT pid=(\d+) call=(\d+) api=(\w+) object=(0x[0-9a-f]+) metadata_only=1')
REGION = re.compile(r'SWEET SNAPSHOT REGION pid=(\d+) call=(\d+) width=(\d+) height=(\d+) depth=(\d+) row=(\d+) image=(\d+) mip=(\d+) slice=(\d+) pixels_read=0')


def inspect(text, token, method_observer=False):
    live = 'SWEET LOOP LIVE proto=1 token='+token
    ack = 'SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live) == 1 and text.count(ack) == 1
    begin, finish = text.index(live), text.index(ack)
    assert begin < finish
    scope = text[begin:finish]
    assert INSTALLED in scope and 'SWEET POSTDRAW observer installed=0' not in scope
    originals = {}
    for match in ORIGINAL.finditer(scope):
        pid, api, image, offset, resolved = match.groups()
        pid = int(pid)
        assert pid > 0 and api in APIS and resolved == '1'
        entries = originals.setdefault(pid, {})
        assert api not in entries
        entries[api] = dict(image=image, offset=offset, resolved=True)
    assert originals and all(set(entries) == set(APIS) for entries in originals.values())

    method_initialization, methods, method_coverage = {}, {}, {}
    if method_observer:
        for match in METHOD_INIT.finditer(scope):
            pid, at = map(int, match.groups())
            assert pid in originals and pid not in method_initialization
            method_initialization[pid] = at
        for match in METHOD.finditer(scope):
            pid, api, present, owned, installed, arguments = match.groups()
            pid, arguments = int(pid), int(arguments)
            assert pid in method_initialization and api in METHOD_APIS
            entries = methods.setdefault(pid, {})
            assert api not in entries
            if installed == '1':
                assert present == owned == '1' and arguments == METHOD_ARGUMENTS[api]
            entries[api] = dict(present=present == '1', owned=owned == '1', installed=installed == '1', arguments=arguments)
        for match in METHOD_CONFIG.finditer(scope):
            pid, installed = map(int, match.groups())
            assert pid in methods and pid not in method_coverage and set(methods[pid]) == set(METHOD_APIS)
            assert 0 <= installed <= 8 and installed == sum(entry['installed'] for entry in methods[pid].values())
            method_coverage[pid] = installed
        assert set(originals) == set(method_initialization) == set(methods) == set(method_coverage)
        for label, pattern in (('INIT', METHOD_INIT), ('METHOD', METHOD), ('OBJECT', OBJECT), ('REGION', REGION), ('observer', METHOD_CONFIG)):
            assert scope.count('SWEET SNAPSHOT '+label+' ') == len(list(pattern.finditer(scope)))

    events = sorted([(match.start(), 'begin', match) for match in BEGIN.finditer(scope)] +
                    [(match.start(), 'end', match) for match in END.finditer(scope)] +
                    [(match.start(), 'image', match) for match in IMAGE.finditer(scope)] +
                    [(match.start(), 'object', match) for match in OBJECT.finditer(scope)] +
                    [(match.start(), 'region', match) for match in REGION.finditer(scope)])
    records, stacks = {}, {}
    for _, kind, match in events:
        if kind == 'begin':
            pid, tid, call, root, depth = map(int, match.groups()[:5])
            api, at = match[6], int(match[7])
            key = (pid, call)
            assert pid in originals and tid > 0 and 1 <= root <= call <= 64 and api in APIS+METHOD_APIS
            if api in METHOD_APIS:
                assert method_observer and methods[pid][api]['installed'] and at >= method_initialization[pid]
            assert key not in records
            stack = stacks.setdefault((pid, tid), [])
            assert depth == len(stack)+1
            assert root == (records[stack[0]]['root'] if stack else call)
            if stack:
                assert at >= records[stack[-1]]['begin_ns']
            records[key] = dict(pid=pid, tid=tid, call=call, root=root, depth=depth,
                                api=api, begin_ns=at, original_forwarded=True, complete=False)
            if method_observer:
                records[key]['since_snapshot_observer_init_ns'] = at-method_initialization[pid]
            stack.append(key)
        elif kind == 'image':
            pid, call, width, height, bpp, pitch, model, nil = map(int, match.groups())
            record = records[(pid, call)]
            assert record['api'] == 'add_image' and not record['complete'] and 'image' not in record
            record['image'] = dict(width=width, height=height, bits_per_pixel=bpp,
                                   pitch=pitch, color_model=model, properties_nil=bool(nil), pixels_read=False)
        elif kind == 'object':
            pid, call, api, object_tag = match.groups()
            record = records[(int(pid), int(call))]
            assert method_observer and api in METHOD_APIS and record['api'] == api and not record['complete'] and 'object_tag' not in record
            record['object_tag'] = object_tag
        elif kind == 'region':
            pid, call, width, height, depth, row, image, level, slice = map(int, match.groups())
            record = records[(pid, call)]
            assert method_observer and record['api'] in ('read', 'read_full') and not record['complete'] and 'read_region' not in record
            record['read_region'] = dict(width=width, height=height, depth=depth, row=row, image=image,
                                         mipmap_level=level, slice=slice, pixels_read_by_observer=False)
        else:
            pid, tid, call = map(int, match.groups()[:3])
            api, wall, cpu, valid, outcome, size, nil = match.groups()[3:]
            wall, cpu, size = map(int, (wall, cpu, size))
            record = records[(pid, call)]
            stack = stacks[(pid, tid)]
            assert stack and stack[-1] == (pid, call)
            assert record['tid'] == tid and record['api'] == api and not record['complete']
            assert outcome in (('true', 'false', 'throw') if api == 'finalize' else ('return', 'throw'))
            assert valid == '1' or cpu == 0
            assert api == 'png' or size == 0
            assert outcome == 'return' or size == 0
            assert api in ('png', 'colorsync', 'pipeline', 'library_url', 'library_bundle', 'snapshot') or nil == '0'
            record.update(complete=True, wall_ns=wall, thread_cpu_ns=cpu, cpu_valid=valid == '1',
                          outcome=outcome, output_bytes=size, returned_nil=nil == '1', original_behavior_preserved=True)
            stack.pop()
    # Do not quietly accept a tee-truncated line as an absent observation.
    for label, pattern in (('BEGIN', BEGIN), ('END', END), ('IMAGE', IMAGE), ('ORIGINAL', ORIGINAL)):
        assert scope.count('SWEET POSTDRAW '+label+' ') == len(list(pattern.finditer(scope)))
    assert scope.count(INSTALLED) == len(originals)
    for record in records.values():
        if record['api'] in METHOD_APIS:
            assert 'object_tag' in record
            if record['api'] in ('read', 'read_full'):
                assert 'read_region' in record
    complete = [record for record in records.values() if record['complete']]
    incomplete = [record for record in records.values() if not record['complete']]
    return dict(original_snapshot_observer_configured=True, source_references=originals,
                snapshot_method_observer_requested=method_observer, snapshot_method_metadata=methods,
                snapshot_method_coverage=method_coverage, snapshot_observer_init_ns=method_initialization,
                elapsed_since_observer_init_is_not_full_extension_budget=True,
                observed_apis=sorted(set(record['api'] for record in records.values())),
                complete_samples=len(complete), incomplete_samples=len(incomplete),
                records=list(records.values()), pending_calls=incomplete,
                longest_completed_inclusive_calls=sorted(complete, key=lambda record:record['wall_ns'], reverse=True)[:8],
                nested_durations_inclusive_not_additive=True,
                absent_API_calls_do_not_prove_absent_work=True,
                full_UI_or_root_cause_not_inferred=True, post_ACK_exit_period_excluded=True)


if __name__ == '__main__':
    import argparse
    import hashlib
    import json
    from pathlib import Path
    from return_evidence import inspect_return
    parser = argparse.ArgumentParser()
    parser.add_argument('--directory', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    folder = Path(args.directory)
    row = json.loads((folder/'results.json').read_text(encoding='utf-8'))[-1]
    artifact = row['artifact']
    assert not row['simulated'] and artifact['trial_mode'] == 'gpu'
    assert artifact['exception_observer'].get('original_postdraw_observer')
    raw = (folder/'cycle-01/serial.raw.log').read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    assert digest == row['log_sha256']
    assert hashlib.sha256(raw[:row['checkpoint_log_bytes']]).hexdigest() == row['checkpoint_log_sha256']
    returned = inspect_return(raw, artifact['token'], row['checkpoint_log_bytes'], require_drain=True)
    assert returned['complete'] and row['fastboot_returned'] and row['reset_to_fastboot_seconds'] >= 30
    report = inspect(raw.decode('ascii', errors='replace'), artifact['token'],
                     method_observer=artifact['exception_observer'].get('snapshot_method_observer', False))
    report.update(token=artifact['token'], raw_sha256=digest, source=str(folder.resolve()),
                  original_result=row['result'], original_reason=row['reason'], strict_return=returned,
                  original_failed_stage_not_accepted=row['result'] != 'PASS')
    output = Path(args.output)
    assert not output.exists()
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('token', 'original_result', 'complete_samples', 'incomplete_samples', 'observed_apis')}))
