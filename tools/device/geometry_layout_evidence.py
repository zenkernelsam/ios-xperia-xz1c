"""Decode one original native UIKit layout capture without device operations."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re


def fields(payload):
    result = {}
    for word in payload.split():
        if '=' not in word:
            raise ValueError('Incomplete layout record')
        key, value = word.split('=', 1)
        if key in result or not value:
            raise ValueError('Duplicate or empty layout field')
        result[key] = value
    return result


def vector(value, length):
    result = [float(x) for x in value.split(',')]
    if len(result) != length or not all(math.isfinite(x) and abs(x) <= 32768 for x in result):
        raise ValueError('Invalid layout geometry')
    return result


def decode(raw, token):
    if not re.fullmatch('[0-9a-f]{16}', token):
        raise ValueError('Invalid current token')
    live = 'SWEET LOOP LIVE proto=1 token=' + token
    ack = 'SWEET LOOP ACK FASTBOOT token=' + token
    if raw.count(live) != 1 or raw.count(ack) != 1:
        raise ValueError('Layout needs one original LIVE and ACK')
    begin_at, end_at = raw.index(live), raw.index(ack)
    if begin_at >= end_at:
        raise ValueError('Wrong current-boot interval')
    records = list(re.finditer(r'SWEET GLAYOUT ([A-Z]+) ([^\r\n]*)', raw[begin_at:end_at]))
    if len(records) < 2 or records[0][1] != 'BEGIN' or records[-1][1] != 'END':
        raise ValueError('Missing original layout boundary')
    if sum(x[1] == 'BEGIN' for x in records) != 1 or sum(x[1] == 'END' for x in records) != 1:
        raise ValueError('Repeated layout snapshot')
    header, footer = fields(records[0][2]), fields(records[-1][2])
    if header != {'rev': '1', 'after_ctor': '135', 'points': '360,640', 'pixels': '720,1280', 'scale': '2'}:
        raise ValueError('Unbound layout profile or observation time')
    if set(footer) != {'rev', 'visited', 'recorded', 'grids', 'refused', 'truncated'} or footer['rev'] != '1':
        raise ValueError('Incomplete layout summary')
    counts = {k: int(v) for k, v in footer.items() if k != 'rev'}
    if not (0 < counts['visited'] <= 2048 and 0 < counts['recorded'] <= 384 and
            0 <= counts['grids'] <= 8 and counts['refused'] >= 0 and counts['truncated'] in (0, 1)):
        raise ValueError('Invalid layout counts')
    views, rectangles, grids, insets, refusals = {}, {}, {}, {}, []
    expected = {'VIEW': {'n', 'parent', 'depth', 'class'}, 'RECT': {'n', 'frame', 'screen'},
                'GRID': {'n', 'rows', 'cols', 'spacing', 'list', 'valid'}, 'INSETS': {'n', 'portrait'}}
    for record in records[1:-1]:
        kind, payload = record[1], record[2]
        if kind == 'REFUSE':
            if payload != 'getter_exception':
                raise ValueError('Unknown layout refusal')
            refusals.append(payload)
            continue
        if kind not in expected:
            raise ValueError('Unknown layout record')
        value = fields(payload)
        if set(value) != expected[kind]:
            raise ValueError('Incomplete original layout row')
        number = int(value['n'])
        if not 1 <= number <= counts['visited']:
            raise ValueError('Foreign layout node')
        table = {'VIEW': views, 'RECT': rectangles, 'GRID': grids, 'INSETS': insets}[kind]
        if number in table:
            raise ValueError('Repeated layout node record')
        if kind == 'VIEW':
            parent, depth = int(value['parent']), int(value['depth'])
            if not (0 <= parent < number and 0 <= depth <= 64 and 0 < len(value['class']) <= 64):
                raise ValueError('Invalid original view metadata')
            table[number] = {'parent': parent, 'depth': depth, 'class': value['class']}
        elif kind == 'RECT':
            table[number] = {'frame': vector(value['frame'], 4), 'window_rectangle': vector(value['screen'], 4)}
        elif kind == 'GRID':
            rows, columns, valid = int(value['rows']), int(value['cols']), int(value['valid'])
            if not (0 <= rows <= 2048 and 0 <= columns <= 2048 and valid in (0, 1)):
                raise ValueError('Invalid original grid values')
            table[number] = {'rows': rows, 'columns': columns, 'spacing': vector(value['spacing'], 2),
                             'list_size': vector(value['list'], 2), 'valid': bool(valid)}
        else:
            table[number] = vector(value['portrait'], 4)
    if len(views) != counts['recorded'] or set(views) != set(rectangles):
        raise ValueError('Missing original view/rectangle pair')
    if sum(x['valid'] for x in grids.values()) != counts['grids']:
        raise ValueError('Incomplete original grid inventory')
    if len(refusals) > counts['refused'] or any(n not in grids for n in insets):
        raise ValueError('Invalid layout refusal/inset inventory')
    complete = (counts['grids'] > 0 and counts['refused'] == counts['truncated'] == 0 and
                all(n in insets for n, x in grids.items() if x['valid']))
    return {'revision': 1, 'token': token, 'current_LIVE_to_ACK_only': True, 'capture_complete': True,
            'counts': counts, 'views': {str(n): dict(v, **rectangles[n]) for n, v in views.items()},
            'grids': {str(n): dict(v, portrait_insets=insets.get(n)) for n, v in grids.items()},
            'refusals': refusals, 'grid_fields_fully_observed': complete,
            'window_coordinates_from_original_convertRect_toView_nil': True,
            'original_UI_layout_success_or_optical_FPS_NOT_inferred': True}


def inspect(directory):
    directory = Path(directory).resolve()
    rows = json.loads((directory / 'results.json').read_text())
    if len(rows) != 1:
        raise ValueError('Not one original trial')
    row, raw_path = rows[0], directory / 'cycle-01/serial.raw.log'
    artifact = row['artifact']
    raw = raw_path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != row['log_sha256'] or not row['return_log_complete']:
        raise ValueError('Original raw/return binding differs')
    if artifact.get('display_geometry_binding', {}).get('profile') != 'xz1c-native-2x':
        raise ValueError('Not an original native-profile trial')
    manifest = json.loads((Path(artifact['ram_package']) / 'manifest.json').read_text())
    binding = manifest['geocompat_build_binding']
    if binding.get('read_only_layout_observer_revision') != 1 or binding.get('layout_snapshot_once_after_constructor_seconds') != 135:
        raise ValueError('Missing original compiled layout binding')
    report = decode(raw.decode('utf-8', errors='replace'), artifact['token'])
    report.update(source=str(directory), raw_sha256=digest, raw_bytes=len(raw),
                  image_sha256=artifact['sha256'], kernel_sha256=artifact['kernel_sha256'],
                  geocompat_build_binding=binding, original_controller_result=row['result'])
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    report = inspect(args.directory)
    output = Path(args.output)
    with output.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print(json.dumps({'output': str(output), 'counts': report['counts'],
                      'grid_fields_fully_observed': report['grid_fields_fully_observed'], 'device_operations': 0}))


if __name__ == '__main__':
    main()
