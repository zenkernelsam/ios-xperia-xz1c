"""Inspect original snapshot timing without treating a timeout as UI success."""
import re

APIS = ('png', 'add_image', 'finalize', 'colorsync')
BEGIN = re.compile(r'SWEET POSTDRAW BEGIN pid=(\d+) tid=(\d+) call=(\d+) root=(\d+) depth=(\d+) api=(\w+) at_ns=(\d+) original_forwarded=1')
END = re.compile(r'SWEET POSTDRAW END pid=(\d+) tid=(\d+) call=(\d+) api=(\w+) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) outcome=(\w+) bytes=(\d+) returned_nil=([01]) original_behavior=1')
IMAGE = re.compile(r'SWEET POSTDRAW IMAGE pid=(\d+) call=(\d+) width=(\d+) height=(\d+) bpp=(\d+) pitch=(\d+) color_model=(-?\d+) properties_nil=([01]) pixels_read=0')
ORIGINAL = re.compile(r'SWEET POSTDRAW ORIGINAL pid=(\d+) api=(\w+) image=(\S+) offset=(0x[0-9a-f]+) resolved=([01])')
INSTALLED = 'SWEET POSTDRAW observer installed=1 interpose_pairs=4 limit=64 metadata_only=1 original_arguments_results=1'


def inspect(text, token):
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

    events = sorted([(match.start(), 'begin', match) for match in BEGIN.finditer(scope)] +
                    [(match.start(), 'end', match) for match in END.finditer(scope)] +
                    [(match.start(), 'image', match) for match in IMAGE.finditer(scope)])
    records, stacks = {}, {}
    for _, kind, match in events:
        if kind == 'begin':
            pid, tid, call, root, depth = map(int, match.groups()[:5])
            api, at = match[6], int(match[7])
            key = (pid, call)
            assert pid in originals and tid > 0 and 1 <= root <= call <= 64 and api in APIS
            assert key not in records
            stack = stacks.setdefault((pid, tid), [])
            assert depth == len(stack)+1
            assert root == (records[stack[0]]['root'] if stack else call)
            if stack:
                assert at >= records[stack[-1]]['begin_ns']
            records[key] = dict(pid=pid, tid=tid, call=call, root=root, depth=depth,
                                api=api, begin_ns=at, original_forwarded=True, complete=False)
            stack.append(key)
        elif kind == 'image':
            pid, call, width, height, bpp, pitch, model, nil = map(int, match.groups())
            record = records[(pid, call)]
            assert record['api'] == 'add_image' and not record['complete'] and 'image' not in record
            record['image'] = dict(width=width, height=height, bits_per_pixel=bpp,
                                   pitch=pitch, color_model=model, properties_nil=bool(nil), pixels_read=False)
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
            assert api in ('png', 'colorsync') or nil == '0'
            record.update(complete=True, wall_ns=wall, thread_cpu_ns=cpu, cpu_valid=valid == '1',
                          outcome=outcome, output_bytes=size, returned_nil=nil == '1', original_behavior_preserved=True)
            stack.pop()
    # Do not quietly accept a tee-truncated line as an absent observation.
    for label, pattern in (('BEGIN', BEGIN), ('END', END), ('IMAGE', IMAGE), ('ORIGINAL', ORIGINAL)):
        assert scope.count('SWEET POSTDRAW '+label+' ') == len(list(pattern.finditer(scope)))
    assert scope.count(INSTALLED) == len(originals)
    complete = [record for record in records.values() if record['complete']]
    incomplete = [record for record in records.values() if not record['complete']]
    return dict(original_snapshot_observer_configured=True, source_references=originals,
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
    report = inspect(raw.decode('ascii', errors='replace'), artifact['token'])
    report.update(token=artifact['token'], raw_sha256=digest, source=str(folder.resolve()),
                  original_result=row['result'], original_reason=row['reason'], strict_return=returned,
                  original_failed_stage_not_accepted=row['result'] != 'PASS')
    output = Path(args.output)
    assert not output.exists()
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key:report[key] for key in ('token', 'original_result', 'complete_samples', 'incomplete_samples', 'observed_apis')}))
