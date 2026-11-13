"""Supplement an interrupted controller verdict without rewriting it as PASS."""
import argparse
import hashlib
import json
from pathlib import Path
from controller import Artifact, write_json
from package_trial import prove_payload
from return_evidence import inspect_return
from exception_evidence import verify as verify_exception
from policy import unique_fastboot


def sha(data):return hashlib.sha256(data).hexdigest()


def verify(directory, health_directory):
    root=Path(directory);health_root=Path(health_directory)
    original=(root/'results.json').read_bytes();row=json.loads(original)[-1];a=row['artifact']
    assert row['result']=='STOP_ERROR' and row['simulated'] is False and row['live']
    assert row['boot_dispatches']==row['reset_requests']==1
    Artifact(a).validate();payload=prove_payload(a)
    wire=row['boot_wire']
    assert wire['serial']=='BH905ND69G' and wire['retries']==0 and wire['image_sha256']==a['sha256']
    assert wire['download_command_attempts']==wire['boot_command_attempts']==1
    raw=(root/'cycle-01/serial.raw.log').read_bytes()
    assert sha(raw[:row['checkpoint_log_bytes']])==row['checkpoint_log_sha256']
    returned=inspect_return(raw,a['token'],row['checkpoint_log_bytes'],require_drain=True)
    assert returned['complete']
    health_bytes=(health_root/'health.json').read_bytes();health=json.loads(health_bytes)
    assert health['result']=='PASS' and health['serial']=='BH905ND69G'
    assert health['independent_health_only'] and health['previous_trial_result_unchanged']
    assert health['download_commands']==health['boot_commands']==health['reset_commands']==0
    samples=health['samples'];assert len(samples)>=2 and all(s['healthy'] for s in samples)
    elapsed=[s['elapsed_seconds'] for s in samples]
    assert elapsed==sorted(elapsed) and elapsed[-1]-elapsed[0]>=30
    assert all(0<b-a<5 for a,b in zip(elapsed,elapsed[1:]))
    assert abs(health['continuous_health_seconds']-(elapsed[-1]-elapsed[0]))<0.001
    events=[json.loads(line) for line in (health_root/'events.jsonl').read_text().splitlines()]
    probes=[e for e in events if e['state'] in ('usb_identity','inventory','health')]
    assert len(probes)==len(samples)*3
    logs={}
    for path in sorted(health_root.glob('command-*.log')):
        data=path.read_bytes();logs[path.name]=sha(data)
    assert len(logs)==len(events)
    for i,event in enumerate(events):
        assert event['code']==0
        text=(health_root/f'command-{i:03d}.log').read_text()
        if event['state']=='usb_identity':assert text.strip()=='BH905ND69G'
        elif event['state']=='inventory':unique_fastboot(text)
        elif event['state']=='health':
            assert event['argv'][1:]==['-s','BH905ND69G','getvar','version-bootloader']
            assert 'version-bootloader:' in text
    assert (root/'results.json').read_bytes()==original
    return dict(original_controller_result=row['result'],original_controller_error=row['error'],
        original_results_sha256=sha(original),original_run=str(root.resolve()),token=a['token'],
        image_sha256=a['sha256'],kernel_sha256=a['kernel_sha256'],payload_audit=payload,
        single_download_boot_verified=True,return_complete=returned,
        independent_health=health,health_results_sha256=sha(health_bytes),health_command_hashes=logs,
        original_result_not_promoted_to_PASS=True,exceptions=verify_exception(raw.decode('ascii','replace'),a['token']),
        full_UI_and_peripheral_goal_not_achieved=True)


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('directory');parser.add_argument('--health',required=True)
    parser.add_argument('--output',required=True);args=parser.parse_args()
    report=verify(args.directory,args.health);write_json(args.output,report)
    print(json.dumps(dict(token=report['token'],original_result=report['original_controller_result'],
        raw_return_complete=report['return_complete']['complete'],
        health_seconds=report['independent_health']['continuous_health_seconds'],
        complete_exceptions=len(report['exceptions']['records']),incomplete_exceptions=len(report['exceptions']['incomplete_records']))))
