"""Exercise complete model identity comparison, nesting and actual mode binding."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/device'))
from native_model_graph import decode, compare
from native_grid_control import apply, validate
from geometry_layout_evidence import decode as decode_layout


def fixture(fitted):
    records = []
    def model(n, kind, parent, index, sub, folder, entries):
        records.append(('MODEL', f'n={n} kind={kind} parent={parent} index={index} list={sub} folder={folder} count={len(entries)}'))
        for i, (cls, app, identifier) in enumerate(entries):
            records.append(('ICON', f'n={n} index={i} class={cls} application={app} id={identifier}'))
    app = lambda name: ('SBApplicationIcon', 1, 'com.example.' + name)
    first = [app('a'), ('SBWidgetIcon', 0, '-'), ('SBFolderIcon', 0, '-')] + ([] if fitted else [app('b')])
    second = ([app('b')] if fitted else []) + [app('c')]
    model(1, 'root', 0, 0, 0, '-', first)
    records.append(('LINK', 'n=1 index=2 folder=stable-folder lists=1'))
    model(2, 'root', 0, 0, 0, '-', second)
    model(3, 'dock', 0, 0, 0, '-', [app('dock')])
    model(2049, 'folder', 1, 2, 0, 'stable-folder', [app('nested')])
    records.append(('MODELS', 'pages=4 icons=7 applications=5'))
    views = {1: {'class': 'SBIconListView'}, 2: {'class': 'SBIconListView'}, 3: {'class': 'SBDockIconListView'}}
    rectangles = {1: {'frame': [0,0,360,640]}, 2: {'frame': [360,0,360,640]}, 3: {'frame': [0,0,360,80]}}
    models, summary, links = decode(records, views, rectangles, {})
    report = {'revision': 3, 'native_grid_value': int(fitted), 'capture_complete': True, 'grid_fields_fully_observed': True,
              'kernel_sha256': 'same', 'geocompat_build_binding': {'same': True}, 'models': models, 'model_summary': summary,
              'model_links': links, 'token': 'fitted' if fitted else 'original', 'raw_sha256': 'fixture-not-hardware'}
    return records, views, rectangles, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    records, views, rectangles, original = fixture(False)
    fitted = fixture(True)[3]
    good = compare(fitted, original)
    assert good['complete_original_model_identity_verified'] and good['original_full_application_entries'] == 5
    token = '1111111111111111'
    wire = ['SWEET LOOP LIVE proto=1 token=' + token,
            'SWEET GLAYOUT BEGIN rev=3 after_ctor=120 points=360,640 pixels=720,1280 scale=2 grid=0']
    for n, name, width, height in ((1,'SBIconListView',360,640),(2,'SBIconListView',360,640),(3,'SBDockIconListView',360,80)):
        wire += [f'SWEET GLAYOUT VIEW n={n} parent=0 depth=0 class={name}',
                 f'SWEET GLAYOUT RECT n={n} frame=0,0,{width},{height} screen=0,0,{width},{height}',
                 f'SWEET GLAYOUT GRID n={n} rows={6 if n<3 else 1} cols=4 spacing=1,1 list=320,693 valid=1',
                 f'SWEET GLAYOUT INSETS n={n} portrait=0,0,0,0']
    wire += ['SWEET GLAYOUT ' + kind + ' ' + value for kind, value in records]
    wire += ['SWEET GLAYOUT END rev=3 visited=3 recorded=3 grids=3 refused=0 truncated=0 write_failures=0',
             'SWEET LOOP ACK FASTBOOT token=' + token]
    parsed = decode_layout('\n'.join(wire), token)
    assert parsed['revision']==3 and parsed['models']['2049']['kind']=='folder' and parsed['model_summary']['applications']==5
    faults = {}
    for name, change in {
        'root_application': lambda data: data['models']['1']['icons'][0].update(identifier='different'),
        'folder_application': lambda data: data['models']['2049']['icons'][0].update(identifier='different'),
        'dock_application': lambda data: data['models']['3']['icons'][0].update(identifier='different'),
        'folder_identity': lambda data: data['model_links']['1:2'].update(folder='different'),
    }.items():
        data = copy.deepcopy(fitted);change(data)
        assert not compare(data, original)['complete_original_model_identity_verified']
        faults[name] = True
    broken_records = [x for x in records if not (x[0] == 'MODEL' and x[1].startswith('n=2049 '))]
    try: decode(broken_records, views, rectangles, {})
    except ValueError: faults['missing_nested_model'] = True
    else: raise AssertionError('Missing nested model accepted')
    words = ['sweetenv=backboardd:DYLD_INSERT_LIBRARIES=/dev/sglram/m;SpringBoard:DYLD_INSERT_LIBRARIES=/dev/sglram/g,GEO_FIX=1']
    for mode in ('original', 'fitted'):
        actual, binding = apply(words, mode)
        record = {'trial_mode': 'gpu', 'display_geometry_binding': {'profile': 'xz1c-native-2x'},
                  'native_grid_control': binding, 'cmdline': ' '.join(actual)}
        package = {'geocompat_build_binding': {'revision': 3, 'native_root_grid_mode_control_revision': 1}}
        assert validate(record, package) == binding
        damaged = copy.deepcopy(record);damaged['cmdline'] = damaged['cmdline'].replace('GEO_GRID=' + str(binding['value']), 'GEO_GRID=' + str(1 - binding['value']))
        try: validate(damaged, package)
        except ValueError: faults['boot_env_' + mode] = True
        else: raise AssertionError('Wrong native mode accepted')
    paths = [Path(__file__), ROOT/'tools/device/native_model_graph.py', ROOT/'tools/device/native_grid_control.py']
    report = {'source_sha256': {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
              'full_synthetic_model_identity_verified': True, 'revision3_wire_graph_accepted':True, 'root_page_reflow_preserves_containers': True, 'faults_refused': faults,
              'synthetic_fixture_not_device_UI_evidence': True, 'device_operations': 0}
    with (args.output/'manifest.json').open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, indent=2);stream.write('\n')
    print(json.dumps(report))


if __name__ == '__main__': main()
