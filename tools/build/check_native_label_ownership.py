"""Exercise complete ancestry and same-position foreign labels using author fixtures."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/device'))
from geometry_layout_evidence import decode
from native_grid_profile_evidence import evaluate
from native_grid_control import apply, validate

TOKEN = '5555555555555555'


def fixture(fitted=True, duplicate_owned=False):
    # Synthetic rectangles deliberately overlap across different trees. They
    # are author input, never a reconstruction or amendment of device R20.
    rows = 5 if fitted else 6
    y, label_y, app_y = (61, 204.5, 243) if fitted else (52, 195.5, 205.667)
    nodes = {1: (0, 0, 0), 2: (1, 1, 2), 3: (1, 1, 2), 4: (1, 1, 2),
             5: (2, 2, 1), 6: (2, 2, 1), 7: (1, 1, 0), 8: (5, 3, 0),
             9: (8, 4, 3), 10: (7, 2, 3)}
    views = {1: ('UIWindow', [0, 0, 360, 640]),
             2: ('SBIconListView', [0, 0, 360, 640]),
             3: ('SBIconListView', [360, 0, 360, 640]),
             4: ('SBDockIconListView', [0, 560, 360, 80]),
             5: ('SBIconView', [34, y, 141, 155]),
             6: ('SBIconView', [19, app_y, 56, 70]),
             9: ('SBIconLegibilityLabelView', [72.5, label_y, 65, 21]),
             10: ('SBIconLegibilityLabelView', [72.5, label_y, 65, 21])}
    owners = {9: (5, 2, 4), 10: (0, 0, 2)}
    if duplicate_owned:
        nodes[11] = (5, 3, 3)
        views[11] = views[9]
        owners[11] = (5, 2, 3)
    wire = ['SWEET LOOP LIVE proto=1 token=' + TOKEN,
            f'SWEET GLAYOUT BEGIN rev=4 after_ctor=120 points=360,640 pixels=720,1280 scale=2 grid={int(fitted)}']
    for n, (parent, depth, role) in nodes.items():
        wire.append(f'SWEET GLAYOUT TREE n={n} parent={parent} depth={depth} role={role}')
        if n in owners:
            icon, listing, hops = owners[n]
            wire.append(f'SWEET GLAYOUT OWNER n={n} icon={icon} list={listing} hops={hops} complete=1')
        if n in views:
            name, rectangle = views[n]
            value = ','.join(map(str, rectangle))
            wire += [f'SWEET GLAYOUT VIEW n={n} parent={parent} depth={depth} class={name}',
                     f'SWEET GLAYOUT RECT n={n} frame={value} screen={value}']
            if role == 2:
                wire += [f'SWEET GLAYOUT GRID n={n} rows={rows if n < 4 else 1} cols=4 spacing=1,1 list=320,693 valid=1',
                         f'SWEET GLAYOUT INSETS n={n} portrait=0,0,0,0']
    app = lambda name: ('SBApplicationIcon', 1, 'com.example.' + name)
    models = [(2, 'root', 0, 0, '-', [app('a'), ('SBWidgetIcon', 0, '-')]),
              (3, 'root', 0, 0, '-', [app('b'), ('SBFolderIcon', 0, '-')]),
              (4, 'dock', 0, 0, '-', [app('dock')]),
              (2049, 'folder', 3, 1, 'stable-folder', [app('nested')])]
    for n, kind, parent, index, folder, icons in models:
        wire.append(f'SWEET GLAYOUT MODEL n={n} kind={kind} parent={parent} index={index} list=0 folder={folder} count={len(icons)}')
        for i, (cls, is_app, identifier) in enumerate(icons):
            wire.append(f'SWEET GLAYOUT ICON n={n} index={i} class={cls} application={is_app} id={identifier}')
        if n == 3:
            wire.append('SWEET GLAYOUT LINK n=3 index=1 folder=stable-folder lists=1')
    wire += ['SWEET GLAYOUT MODELS pages=4 icons=6 applications=4',
             f'SWEET GLAYOUT END rev=4 visited={len(nodes)} recorded={len(views)} grids=3 refused=0 truncated=0 write_failures=0 tree_nodes={len(nodes)} owners={len(owners)}',
             'SWEET LOOP ACK FASTBOOT token=' + TOKEN]
    return '\n'.join(wire)


def report(raw):
    result = decode(raw, TOKEN)
    result.update(kernel_sha256='author-fixture', raw_sha256=hashlib.sha256(raw.encode()).hexdigest(),
                  geocompat_build_binding={'same': True}, original_controller_result='fixture',
                  native_grid_control={'value': result['native_grid_value']})
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    raw, baseline = fixture(), report(fixture(False))
    current = report(raw)
    good = evaluate(current, baseline)
    assert good['result'] == 'PASS' and good['original_label_ancestry_verified']
    assert good['original_label_clearances'][0]['clearance_points'] == 17.5
    assert good['original_application_model_identity_retention_verified']
    assert current['label_owners']['10']['icon'] == current['label_owners']['10']['list'] == 0
    opaque = raw.replace('class=SBIconLegibilityLabelView', 'class=SweetGeoCaptionView')
    assert evaluate(report(opaque), baseline)['result'] == 'PASS'
    faults = {}
    changes = {
        'missing_tree': lambda x: x.replace('SWEET GLAYOUT TREE n=7 parent=1 depth=1 role=0\n', ''),
        'repeated_tree': lambda x: x.replace('SWEET GLAYOUT TREE n=7', 'SWEET GLAYOUT TREE n=6'),
        'self_parent': lambda x: x.replace('TREE n=8 parent=5', 'TREE n=8 parent=8'),
        'foreign_parent': lambda x: x.replace('TREE n=8 parent=5', 'TREE n=8 parent=2049'),
        'wrong_depth': lambda x: x.replace('TREE n=8 parent=5 depth=3', 'TREE n=8 parent=5 depth=9'),
        'unknown_role': lambda x: x.replace('TREE n=8 parent=5 depth=3 role=0', 'TREE n=8 parent=5 depth=3 role=4'),
        'class_role_mismatch': lambda x: x.replace('TREE n=5 parent=2 depth=2 role=1', 'TREE n=5 parent=2 depth=2 role=0'),
        'wrong_owner': lambda x: x.replace('OWNER n=9 icon=5', 'OWNER n=9 icon=6'),
        'wrong_list': lambda x: x.replace('OWNER n=9 icon=5 list=2', 'OWNER n=9 icon=5 list=3'),
        'wrong_hops': lambda x: x.replace('OWNER n=9 icon=5 list=2 hops=4', 'OWNER n=9 icon=5 list=2 hops=3'),
        'unknown_owner': lambda x: x.replace('hops=4 complete=1', 'hops=4 complete=0'),
        'missing_owner': lambda x: x.replace('SWEET GLAYOUT OWNER n=9 icon=5 list=2 hops=4 complete=1\n', ''),
        'missing_owner_field': lambda x: x.replace('OWNER n=9 icon=5 list=2', 'OWNER n=9 icon=5'),
        'wrong_tree_count': lambda x: x.replace('tree_nodes=10', 'tree_nodes=9'),
        'wrong_owner_count': lambda x: x.replace('owners=2', 'owners=1'),
    }
    for name, change in changes.items():
        try:
            decode(change(raw), TOKEN)
        except ValueError:
            faults[name] = True
        else:
            raise AssertionError('Invalid original ancestry accepted: ' + name)
    duplicate = evaluate(report(fixture(duplicate_owned=True)), baseline)
    assert duplicate['result'] == 'REVISE' and 'WIDGET_ORIGINAL_LABEL_AMBIGUOUS' in duplicate['reasons']
    faults['duplicate_within_exact_owner'] = True
    invalid = copy.deepcopy(current)
    invalid['views']['9']['window_rectangle'][1] = -10
    assert evaluate(invalid, baseline)['result'] == 'REVISE'
    faults['owned_label_outside_native_window'] = True
    for mode in ('original', 'fitted'):
        words, bound = apply(['sweetenv=SpringBoard:DYLD_INSERT_LIBRARIES=/dev/sglram/g,GEO_FIX=1'], mode)
        record = {'trial_mode': 'gpu', 'display_geometry_binding': {'profile': 'xz1c-native-2x'},
                  'native_grid_control': bound, 'cmdline': ' '.join(words)}
        assert validate(record, {'geocompat_build_binding': {'revision': 4, 'native_root_grid_mode_control_revision': 1}}) == bound
    paths = [Path(__file__), ROOT/'tools/device/native_label_ownership.py', ROOT/'tools/device/geometry_layout_evidence.py',
             ROOT/'tools/device/native_grid_profile_evidence.py', ROOT/'tools/device/native_model_graph.py']
    proof = {'source_files': {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
             'original_ancestry_and_exact_owner_verified': True, 'same_rectangle_foreign_label_excluded': True,
             'original_class_hierarchy_including_opaque_subclasses_verified': True,
             'duplicate_in_same_owner_still_REVISE': True, 'complete_root_dock_folder_models_preserved': True,
             'faults_refused': faults, 'new_RAM_CPU_GPU_UI_NOT_verified': True,
             'author_fixture_not_device_UI': True, 'device_operations': 0}
    with (args.output/'manifest.json').open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(proof, stream, indent=2); stream.write('\n')
    print(json.dumps(proof))


if __name__ == '__main__':
    main()
