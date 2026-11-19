"""Verify a returned scanout independently while preserving the trial decision."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from controller import Artifact, checked_screen_capture_after, validate_plan
from package_trial import ROOT, prove_payload
from return_evidence import inspect_return
from scanout_snapshot import verify_saved


def sha(data):
    return hashlib.sha256(data).hexdigest()


def verify(directory):
    root = Path(directory).resolve()
    result_bytes = (root/'results.json').read_bytes()
    rows = json.loads(result_bytes)
    assert len(rows) == 1
    row = rows[0]
    artifact = row['artifact']
    assert row['simulated'] is False and row['live'] is True
    assert row['result'] == row['decision'] and row['result'] in ('PASS', 'REVISE', 'STOP')
    assert artifact['trial_mode'] == 'gpu' and artifact['version'] >= 730
    assert row['reset_ack'] is True and row['fastboot_returned'] is True
    assert row['boot_dispatches'] == row['reset_requests'] == 1
    assert row['reset_to_fastboot_seconds'] >= 30
    Artifact(artifact).validate()
    payload = prove_payload(artifact)
    wire = row['boot_wire']
    assert wire['serial'] == 'BH905ND69G' and wire['retries'] == 0
    assert wire['download_command_attempts'] == wire['boot_command_attempts'] == 1
    assert wire['image_sha256'] == artifact['sha256']
    raw = (root/'cycle-01/serial.raw.log').read_bytes()
    assert sha(raw) == row['log_sha256']
    assert sha(raw[:row['checkpoint_log_bytes']]) == row['checkpoint_log_sha256']
    returned = inspect_return(raw, artifact['token'], row['checkpoint_log_bytes'], require_drain=True)
    assert returned['complete']
    commands = [json.loads(line)['command'] for line in
                (root/'cycle-01/serial-commands.jsonl').read_text(encoding='utf-8').splitlines()]
    assert commands == ['frame', 'fastboot '+artifact['token']]
    screen = verify_saved(root/'cycle-01/screens', artifact, row['screen'], raw)
    inputs = json.loads((root/'stage-inputs.json').read_bytes())
    bundle = Path(inputs['bundle'])
    if not bundle.is_absolute():
        bundle = ROOT/bundle
    bundle = bundle.resolve()
    assert bundle.is_relative_to(ROOT/'build')
    bundled = json.loads((bundle/'artifacts.json').read_bytes())['artifacts']
    assert bundled == [artifact]
    plan = json.loads((bundle/'plan.json').read_bytes())
    assert validate_plan(plan) == 1 and plan['screen_required'] is True
    assert plan['camera_required'] is False and plan['screen_timeout_seconds'] == 100
    scheduled = checked_screen_capture_after(artifact['screen_capture_after_seconds'])
    assert plan['screen_capture_after_seconds'] == scheduled
    events = [json.loads(line) for line in (root/'events.jsonl').read_text(encoding='utf-8').splitlines()]
    requested = [event for event in events if event['state'] == 'SCREEN_REQUESTED']
    verified = [event for event in events if event['state'] == 'SCREEN_VERIFIED']
    assert len(requested) == len(verified) == 1
    frame = screen['frames'][0]
    assert requested[0]['token'] == verified[0]['token'] == artifact['token']
    assert requested[0]['command_dispatches'] == 1 and verified[0]['frame_id'] == frame['id']
    assert verified[0]['png_sha256'] == frame['png_sha256']
    points = [event for event in events if event['state'] == 'SCREEN_OBSERVATION_POINT']
    assert len(points) <= 1
    if points:
        assert points[0]['token'] == artifact['token'] and points[0]['elapsed_seconds'] >= scheduled
        assert events.index(points[0]) < events.index(requested[0]) < events.index(verified[0])
    assert any(event['state'] == 'FASTBOOT_HEALTHY' for event in events)
    text = raw.decode('ascii', 'replace')
    live, ack = 'SWEET LOOP LIVE proto=1 token='+artifact['token'], 'SWEET LOOP ACK FASTBOOT token='+artifact['token']
    assert text.count(live) == text.count(ack) == 1
    scope = text[text.index(live):text.index(ack)]
    scope = re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', scope)
    missing = [pattern for pattern in plan['steps'][0]['goals'] if not re.search(pattern, scope)]
    assert (root/'results.json').read_bytes() == result_bytes
    sources = ['tools/device/scanout_trial_evidence.py', 'tools/device/scanout_snapshot.py',
               'tools/device/controller.py', 'tools/device/return_evidence.py',
               'tools/device/package_trial.py', 'tools/diagnostic_candidate.py']
    return dict(source=str(root), token=artifact['token'], version=artifact['version'],
        kernel_sha256=artifact['kernel_sha256'], image_sha256=artifact['sha256'],
        original_result=row['result'], original_reason=row['reason'], original_results_sha256=sha(result_bytes),
        raw_sha256=sha(raw), raw_bytes=len(raw), scheduled_capture_after_seconds=scheduled,
        captured_at_observation_point=bool(points),
        actual_observation_elapsed_seconds=points[0]['elapsed_seconds'] if points else None,
        original_recorded_goal_patterns_not_observed=missing,
        stage_decision_not_promoted=True, single_frame_and_return_command_verified=True,
        screenshot_verified_independently_of_trial_result=True, screen=screen,
        payload_audit=payload, strict_return=returned,
        return_seconds_with_health=row['reset_to_fastboot_seconds'],
        optical_panel_backlight_UI_FPS_not_inferred=True,
        evidence_tools={p:sha((ROOT/p).read_bytes()) for p in sources})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    report = verify(args.directory)
    output = Path(args.output)
    assert not output.exists()
    with output.open('xb') as stream:
        stream.write((json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps({key:report[key] for key in ('token', 'original_result',
        'actual_observation_elapsed_seconds', 'return_seconds_with_health')}, indent=2))
