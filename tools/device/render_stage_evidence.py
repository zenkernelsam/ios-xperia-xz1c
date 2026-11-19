"""Recheck bounded draw-stage metadata from the original current-boot log."""
import argparse
import hashlib
import json
import re
from pathlib import Path

STAGES = ('gate', 'make_current', 'targets', 'buffers', 'samples', 'xr_lut',
          'state', 'submit', 'gpu_finish', 'writeback', 'fragment_sync', 'retire')
REQUEST = re.compile(r'SWEET RENDER REQUEST call=(\d+) width=(\d+) height=(\d+) primitive=(\d+) first=(\d+) count=(\d+) instances=(\d+) ordered_mask=(\d+) buffers=(\d+) textures=(\d+) samplers=(\d+) metadata_only=1$')
BEGIN = re.compile(r'SWEET RENDER STAGE BEGIN call=(\d+) stage=(\w+) at_ns=(\d+) metadata_only=1$')
END = re.compile(r'SWEET RENDER STAGE END call=(\d+) stage=(\w+) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) result=(-?\d+) original_order=1$')
TARGET_BEGIN = re.compile(r'SWEET RENDER TARGET BEGIN call=(\d+) index=([0-7]) format=(\d+) load=([012]) part=(import|snapshot|linear|clear) at_ns=(\d+) metadata_only=1$')
TARGET_END = re.compile(r'SWEET RENDER TARGET END call=(\d+) index=([0-7]) part=(import|snapshot|linear|clear) wall_ns=(\d+) cpu_ns=(\d+) cpu_valid=([01]) result=(-?\d+) original_order=1$')
DRAW = re.compile(r'SWEET AIR RENDER draw width=(\d+) height=(\d+) primitive=(\d+) first=(\d+) count=(\d+) instances=(\d+) target_handle=(\d+) result=(-?\d+) sequence=(\d+) original_pages=1$')
INSTALL = re.compile(r'SWEET EXCEPTION observer installed process=(backboardd|AegirPoster) limit=16 original_chain=1$')
PID = re.compile(r'SWEET SNAPSHOT INIT pid=(\d+) at_ns=\d+ expected=8 metadata_only=1$')


def inspect(text, token, target_observer=False):
    assert re.fullmatch('[0-9a-f]{16}', token)
    live = 'SWEET LOOP LIVE proto=1 token=' + token
    ack = 'SWEET LOOP ACK FASTBOOT token=' + token
    assert text.count(live) == text.count(ack) == 1
    start, stop = text.index(live), text.index(ack)
    assert start < stop
    scope = text[start:stop]
    sessions, current, records, seen = [], {}, [], dict(request=0, begin=0, end=0, target_begin=0, target_end=0)
    for tagged in re.finditer(r'\|(backboardd|AegirPoster)\| ([^\r\n]*)', scope):
        process, line = tagged.groups()
        installed = INSTALL.fullmatch(line)
        if installed:
            assert installed[1] == process
            session = dict(process=process, session=len(sessions) + 1,
                           pid=None, records=[], last_call=0)
            sessions.append(session)
            current[process] = session
            continue
        pid = PID.fullmatch(line)
        if pid:
            assert process == 'AegirPoster' and process in current
            assert current[process]['pid'] is None and int(pid[1]) > 0
            current[process]['pid'] = int(pid[1])
            continue
        request, begin, end, draw, target_begin, target_end = (pattern.fullmatch(line) for pattern in (REQUEST, BEGIN, END, DRAW, TARGET_BEGIN, TARGET_END))
        if line.startswith(('SWEET RENDER REQUEST ', 'SWEET RENDER STAGE ', 'SWEET RENDER TARGET ')):
            assert request or begin or end or target_begin or target_end, 'Truncated or unknown draw-stage record'
        if not any((request, begin, end, draw, target_begin, target_end)):
            continue
        if draw and (process not in current or not current[process]['records']):
            # Only the first 32 draws are observed; ordinary draws may follow.
            continue
        assert process in current, 'Draw stage lacks its process installation'
        session = current[process]
        if request:
            seen['request'] += 1
            fields = list(map(int, request.groups()))
            call = fields[0]
            assert 1 <= call <= 32 and call not in [r['call'] for r in session['records']]
            record = dict(process=process, session=session['session'], pid=session['pid'],
                          call=call, **dict(zip(('width', 'height', 'primitive', 'first', 'count',
                                               'instances', 'ordered_mask', 'buffers', 'textures', 'samplers'), fields[1:])),
                          stages=[], targets=[], draw=None)
            assert record['width'] > 0 and record['height'] > 0 and record['count'] > 0 and record['instances'] > 0
            session['last_call'] = max(session['last_call'], call)
            session['records'].append(record)
            records.append(record)
            continue
        assert session['records']
        if draw:
            candidates = [r for r in session['records'] if r['draw'] is None and r['stages'] and
                          r['stages'][-1]['stage'] == 'retire' and not r['stages'][-1]['complete']]
            if not candidates and any(r['call'] == 32 and r['draw'] is not None for r in session['records']):
                continue
            assert len(candidates) == 1, 'Draw return lacks one observed retiring request'
            record = candidates[0]
        else:
            call = int((begin or end or target_begin or target_end)[1])
            candidates = [r for r in session['records'] if r['call'] == call]
            assert len(candidates) == 1, 'Stage has no unique process/call binding'
            record = candidates[0]
        if target_begin or target_end:
            assert target_observer and record['stages'] and record['stages'][-1]['stage']=='targets' and not record['stages'][-1]['complete']
            if target_begin:
                seen['target_begin']+=1
                call,index,fmt,load,part,at=target_begin.groups();index,fmt,load,at=int(index),int(fmt),int(load),int(at)
                previous=record['targets'][-1] if record['targets'] else None
                assert at>=record['stages'][-1]['begin_ns']
                if previous:
                    assert previous['complete'] and at>=previous['begin_ns']+previous['wall_ns']
                    if index==previous['index']:
                        assert fmt==previous['format'] and load==previous['load_action']
                        assert ['import','snapshot','linear','clear'].index(part)>['import','snapshot','linear','clear'].index(previous['part'])
                    else:assert index>previous['index'] and part=='import'
                else:assert part=='import'
                record['targets'].append(dict(index=index,format=fmt,load_action=load,part=part,begin_ns=at,complete=False))
            else:
                seen['target_end']+=1
                call,index,part,wall,cpu,valid,result=target_end.groups();assert record['targets']
                target=record['targets'][-1];assert int(index)==target['index'] and part==target['part'] and not target['complete']
                assert valid=='1' or int(cpu)==0
                target.update(complete=True,wall_ns=int(wall),thread_cpu_ns=int(cpu),cpu_valid=valid=='1',result=int(result))
        elif begin:
            seen['begin'] += 1
            call, name, at = int(begin[1]), begin[2], int(begin[3])
            assert call == record['call'] and name in STAGES
            previous = record['stages'][-1] if record['stages'] else None
            assert name not in [stage['stage'] for stage in record['stages']]
            if previous:
                assert STAGES.index(name) > STAGES.index(previous['stage'])
                assert name == 'retire' or (previous['complete'] and STAGES.index(name) == STAGES.index(previous['stage']) + 1)
                assert at >= previous['begin_ns'] + previous.get('wall_ns', 0)
            else:
                assert name == 'gate'
            record['stages'].append(dict(stage=name, begin_ns=at, complete=False))
        elif end:
            seen['end'] += 1
            assert record['stages']
            stage = record['stages'][-1]
            call, name, wall, cpu, valid, result = end.groups()
            assert int(call) == record['call'] and name == stage['stage'] and not stage['complete']
            assert valid == '1' or int(cpu) == 0
            if name == 'retire':
                assert record['draw'] is not None and int(result) == record['draw']['result']
            if name=='targets' and target_observer:
                if int(result)==0:
                    assert record['targets'] and all(t['complete'] and t['result']==0 for t in record['targets'])
                    for index in sorted(set(t['index'] for t in record['targets'])):
                        parts=[t for t in record['targets'] if t['index']==index]
                        expected=['import']+(['snapshot','linear'] if 552<=parts[0]['format']<=555 else [])+(['clear'] if parts[0]['load_action']==2 else [])
                        assert [t['part'] for t in parts]==expected
                assert all(t['begin_ns']+t.get('wall_ns',0)<=stage['begin_ns']+int(wall) for t in record['targets'])
            stage.update(complete=True, wall_ns=int(wall), thread_cpu_ns=int(cpu),
                         cpu_valid=valid == '1', result=int(result))
        elif draw:
            assert record['draw'] is None and record['stages'][-1]['stage'] == 'retire'
            values = list(map(int, draw.groups()))
            assert values[:6] == [record[k] for k in ('width', 'height', 'primitive', 'first', 'count', 'instances')]
            record['draw'] = dict(target_handle=values[6], result=values[7], sequence=values[8])
    assert seen['request'] == scope.count('SWEET RENDER REQUEST ')
    assert seen['begin'] == scope.count('SWEET RENDER STAGE BEGIN ')
    assert seen['end'] == scope.count('SWEET RENDER STAGE END ')
    assert seen['target_begin']==scope.count('SWEET RENDER TARGET BEGIN ')
    assert seen['target_end']==scope.count('SWEET RENDER TARGET END ')
    complete, pending = [], []
    for record in records:
        record['complete'] = bool(record['draw'] and record['stages'] and
                                  record['stages'][-1]['stage'] == 'retire' and record['stages'][-1]['complete'])
        record['all_stages_completed_successfully'] = (record['complete'] and record['draw']['result'] == 0 and
            [s['stage'] for s in record['stages']] == list(STAGES) and
            all(s['complete'] and s['result'] == 0 for s in record['stages']))
        (complete if record['complete'] else pending).append(record)
    longest = [dict(process=r['process'], session=r['session'], pid=r['pid'], call=r['call'],
                    width=r['width'], height=r['height'], **s) for r in records for s in r['stages'] if s['complete']]
    return dict(token=token, process_installations=[{k:v for k,v in s.items() if k != 'records'} for s in sessions],
                observed_requests=len(records), complete_draws=len(complete), incomplete_draws=len(pending),
                records=records, pending_draws=pending,
                longest_completed_stages=sorted(longest, key=lambda s:s['wall_ns'], reverse=True)[:12],
                post_ACK_exit_period_excluded=True, metadata_only=True,target_observer_requested=target_observer,
                stage_time_includes_driver_CPU_scheduling_and_wait=True,
                stage_totals_not_added_to_overlapping_snapshot_wait_execute=True,
                GPU_hardware_root_cause_and_UI_FPS_not_inferred=True)


def verify(directory):
    from trial_evidence import verify as verify_trial
    from failed_trial_evidence import verify as verify_failure
    from ram_library import load
    root = Path(directory)
    row = json.loads((root / 'results.json').read_bytes())[-1]
    artifact = row['artifact']
    assert artifact['trial_mode'] == 'gpu' and not row['simulated']
    actual = verify_trial(root, 'gpu') if row['result'] == 'PASS' else verify_failure(root)
    _, _, ram = load(artifact['ram_package'], artifact['ram_library_binding'])
    source = ram['frontend_build_binding']['source_files']['metal_render.c']
    assert source in ('9113b4b09670930eac265c286b1f4685600c566f39d522803992fedce0fad1e0','6a72b11804ba77a871235b4ef5d23b6604f870ed4c581534bc5c7f310a47f895')
    target_observer=ram['frontend_build_binding'].get('render_target_observation_revision')==1
    assert target_observer==(source=='6a72b11804ba77a871235b4ef5d23b6604f870ed4c581534bc5c7f310a47f895')
    raw = (root / 'cycle-01/serial.raw.log').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == actual['log_sha256']
    report = inspect(raw.decode('ascii', 'replace'), artifact['token'],target_observer=target_observer)
    if 'screen_snapshot_binding' in artifact:
        from scanout_snapshot import verify_saved
        report['screen'] = verify_saved(root/'cycle-01/screens', artifact, row['screen'], raw)
    report.update(source=str(root.resolve()), raw_sha256=actual['log_sha256'],
                  kernel_sha256=artifact['kernel_sha256'], GLES_sha256=ram['library_sha256'],
                  render_source_sha256=source, inspector_source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  original_result=row['result'], original_reason=row['reason'],
                  return_seconds_with_health=row['reset_to_fastboot_seconds'],
                  controller_failure_not_promoted=row['result'] != 'PASS')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    report = verify(args.directory)
    output = Path(args.output)
    assert not output.exists()
    output.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps({k:report[k] for k in ('token', 'original_result', 'observed_requests', 'complete_draws', 'incomplete_draws')}))
