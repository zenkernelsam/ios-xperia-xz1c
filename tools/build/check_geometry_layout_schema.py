"""Check native layout revision2 model framing and preserve old raw refusals."""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/device'))
from geometry_layout_evidence import decode


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--original-r16', type=Path, required=True)
    parser.add_argument('--original-r17', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    token = '1111111111111111'
    lines = ['SWEET LOOP LIVE proto=1 token=' + token,
             'SWEET GLAYOUT BEGIN rev=2 after_ctor=120 points=360,640 pixels=720,1280 scale=2']
    for number, x in ((1, 0), (2, 360)):
        lines += [f'SWEET GLAYOUT VIEW n={number} parent=0 depth=0 class=SBIconListView',
                  f'SWEET GLAYOUT RECT n={number} frame={x},0,360,640 screen={x},0,360,640',
                  f'SWEET GLAYOUT GRID n={number} rows=5 cols=4 spacing=-123,-123 list=320,693 valid=1',
                  f'SWEET GLAYOUT INSETS n={number} portrait=57,19,155,19']
    lines += ['SWEET GLAYOUT MODEL n=1 count=2',
              'SWEET GLAYOUT ICON n=1 index=0 class=SBApplicationIcon application=1 id=com.apple.Preferences',
              'SWEET GLAYOUT ICON n=1 index=1 class=SBWidgetIcon application=0 id=com.apple.Preferences',
              'SWEET GLAYOUT MODEL n=2 count=1',
              'SWEET GLAYOUT ICON n=2 index=0 class=SBApplicationIcon application=1 id=com.apple.AppStore',
              'SWEET GLAYOUT MODELS pages=2 icons=3 applications=2',
              'SWEET GLAYOUT END rev=2 visited=2 recorded=2 grids=2 refused=0 truncated=0 write_failures=0',
              'SWEET LOOP ACK FASTBOOT token=' + token]
    raw = '\n'.join(lines)
    good = decode(raw, token)
    assert good['revision'] == 2 and good['grid_fields_fully_observed']
    assert good['models']['2']['icons'][0]['identifier'] == 'com.apple.AppStore'
    faults = {}
    target = 'SWEET GLAYOUT ICON n=2 index=0 class=SBApplicationIcon application=1 id=com.apple.AppStore'
    for name, broken in {
        'missing_icon': raw.replace(target, ''),
        'duplicate_slot': raw.replace(target, target + '\n' + target),
        'duplicate_identity': raw.replace('id=com.apple.AppStore', 'id=com.apple.Preferences'),
        'summary_count': raw.replace('icons=3 applications=2', 'icons=2 applications=2'),
        'unbound_time': raw.replace('after_ctor=120', 'after_ctor=135'),
        'missing_model': raw.replace('SWEET GLAYOUT MODEL n=2 count=1', ''),
    }.items():
        try:
            decode(broken, token)
        except ValueError:
            faults[name] = True
        else:
            raise AssertionError(name)
    errors = decode(raw.replace('write_failures=0', 'write_failures=1'), token)
    assert not errors['grid_fields_fully_observed']
    r16_raw = (args.original_r16 / 'cycle-01/serial.raw.log').read_bytes()
    r17_raw = (args.original_r17 / 'cycle-01/serial.raw.log').read_bytes()
    r16_token = json.loads((args.original_r16 / 'results.json').read_text())[0]['artifact']['token']
    r17_token = json.loads((args.original_r17 / 'results.json').read_text())[0]['artifact']['token']
    assert decode(r16_raw.decode(errors='replace'), r16_token)['counts']['recorded'] == 166
    try:
        decode(r17_raw.decode(errors='replace'), r17_token)
    except ValueError as error:
        assert str(error) == 'Missing original view/rectangle pair'
    else:
        raise AssertionError('Original R17 failure reclassified')
    report = dict(source_sha256={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in (Path(__file__), ROOT / 'tools/device/geometry_layout_evidence.py')},
                  synthetic_revision2_protocol_accepted=True, faults_refused=faults,
                  original_R16_validated=True, original_R17_missing_pair_still_refused=True,
                  writer_failure_prevents_complete_fields=True,
                  original_raw_sha256={'r16': hashlib.sha256(r16_raw).hexdigest(), 'r17': hashlib.sha256(r17_raw).hexdigest()},
                  original_raw_unchanged=True, synthetic_fixture_not_UI_evidence=True, device_operations=0)
    with (args.output / 'manifest.json').open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
