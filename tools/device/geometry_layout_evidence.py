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
    revision = int(header.get('rev', 0))
    after = {1: '135', 2: '120'}.get(revision)
    if after is None or header != {'rev': str(revision), 'after_ctor': after, 'points': '360,640', 'pixels': '720,1280', 'scale': '2'}:
        raise ValueError('Unbound layout profile or observation time')
    footer_keys = {'rev', 'visited', 'recorded', 'grids', 'refused', 'truncated'} | ({'write_failures'} if revision == 2 else set())
    if set(footer) != footer_keys or footer['rev'] != str(revision):
        raise ValueError('Incomplete layout summary')
    counts = {k: int(v) for k, v in footer.items() if k != 'rev'}
    if not (0 < counts['visited'] <= 2048 and 0 < counts['recorded'] <= 384 and
            0 <= counts['grids'] <= 8 and counts['refused'] >= 0 and counts['truncated'] in (0, 1) and counts.get('write_failures', 0) >= 0):
        raise ValueError('Invalid layout counts')
    views, rectangles, grids, insets, refusals = {}, {}, {}, {}, []
    models, model_icons, model_summary = {}, {}, None
    expected = {'VIEW': {'n', 'parent', 'depth', 'class'}, 'RECT': {'n', 'frame', 'screen'},
                'GRID': {'n', 'rows', 'cols', 'spacing', 'list', 'valid'}, 'INSETS': {'n', 'portrait'}}
    for record in records[1:-1]:
        kind, payload = record[1], record[2]
        if revision == 2 and kind in ('MODEL', 'ICON', 'MODELS'):
            value = fields(payload)
            if kind == 'MODEL':
                if set(value) != {'n', 'count'}:
                    raise ValueError('Incomplete original model header')
                number, count = int(value['n']), int(value['count'])
                if number in models or not 1 <= number <= counts['visited'] or not 0 <= count <= 128:
                    raise ValueError('Invalid or duplicate original model')
                models[number] = count
            elif kind == 'ICON':
                if set(value) != {'n', 'index', 'class', 'application', 'id'}:
                    raise ValueError('Incomplete original model icon')
                number, index, application = int(value['n']), int(value['index']), int(value['application'])
                token_key = number, index
                identifier = value['id']
                if token_key in model_icons or not 0 <= index < 128 or application not in (0, 1) or not 0 < len(value['class']) <= 48:
                    raise ValueError('Invalid or duplicate original icon')
                if identifier != '-' and (len(identifier) > 64 or not re.fullmatch('[A-Za-z0-9._-]+', identifier)):
                    raise ValueError('Invalid original application identifier')
                if application and identifier == '-':
                    raise ValueError('Missing original application identifier')
                model_icons[token_key] = {'class': value['class'], 'application': bool(application), 'identifier': identifier}
            else:
                if model_summary is not None or set(value) != {'pages', 'icons', 'applications'}:
                    raise ValueError('Incomplete or repeated model summary')
                model_summary = {k: int(v) for k, v in value.items()}
            continue
        if kind == 'REFUSE':
            if payload != 'getter_exception' and not (revision == 2 and re.fullmatch(r'model_unavailable n=\d+', payload)):
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
    if revision == 2:
        root_nodes = {n for n, v in views.items() if v['class'] == 'SBIconListView' and
                      abs(rectangles[n]['frame'][2] - 360) < .02 and abs(rectangles[n]['frame'][3] - 640) < .02}
        if set(models) != root_nodes or not 0 < len(models) <= 8 or model_summary is None:
            raise ValueError('Incomplete original root-model inventory')
        wanted_icons = {(n, index) for n, count in models.items() for index in range(count)}
        if set(model_icons) != wanted_icons or model_summary != {'pages': len(models), 'icons': len(model_icons),
                                                               'applications': sum(v['application'] for v in model_icons.values())}:
            raise ValueError('Missing original model icon or count')
        applications = [v['identifier'] for v in model_icons.values() if v['application']]
        if len(set(applications)) != len(applications):
            raise ValueError('Repeated original application identity')
    complete = (counts['grids'] > 0 and counts['refused'] == counts['truncated'] == 0 and
                counts.get('write_failures', 0) == 0 and all(n in insets for n, x in grids.items() if x['valid']))
    return {'revision': revision, 'token': token, 'current_LIVE_to_ACK_only': True, 'capture_complete': True,
            'counts': counts, 'views': {str(n): dict(v, **rectangles[n]) for n, v in views.items()},
            'grids': {str(n): dict(v, portrait_insets=insets.get(n)) for n, v in grids.items()},
            'refusals': refusals, 'grid_fields_fully_observed': complete,
            'models': {str(n): {'count': count, 'icons': [model_icons[n, i] for i in range(count)]} for n, count in models.items()},
            'model_summary': model_summary,
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
    revision = binding.get('read_only_layout_observer_revision')
    if revision not in (1, 2) or binding.get('layout_snapshot_once_after_constructor_seconds') != {1: 135, 2: 120}[revision]:
        raise ValueError('Missing original compiled layout binding')
    report = decode(raw.decode('utf-8', errors='replace'), artifact['token'])
    if report['revision'] != revision:
        raise ValueError('Original capture differs from compiled observer revision')
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
