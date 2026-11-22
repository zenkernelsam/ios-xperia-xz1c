"""Verify original native grid reflow and label clearance; never operate a phone."""
import argparse
import json
from pathlib import Path

from geometry_layout_evidence import inspect as inspect_layout


def near(a, b):
    return abs(a - b) <= .02


def root_pages(report):
    result = []
    for number, grid in report['grids'].items():
        view = report['views'].get(number)
        if not view or view['class'] != 'SBIconListView' or grid['columns'] != 4:
            continue
        rectangle = view['window_rectangle']
        if not near(rectangle[2], 360) or not near(rectangle[3], 640):
            continue
        cells = [dict(v, node=n) for n, v in report['views'].items()
                 if v['class'] == 'SBIconView' and str(v['parent']) == number]
        regular = [x for x in cells if near(x['frame'][2], 56) and near(x['frame'][3], 70)]
        widgets = [x for x in cells if near(x['frame'][2], 141) and near(x['frame'][3], 155)]
        result.append(dict(node=number, rectangle=rectangle, grid=grid, regular=regular,
                           widgets=widgets, unsupported_cells=len(cells) - len(regular) - len(widgets)))
    return sorted(result, key=lambda p: p['rectangle'][0])


def evaluate(current, baseline):
    pages, old = root_pages(current), root_pages(baseline)
    reasons = []
    if current['kernel_sha256'] != baseline['kernel_sha256']:
        raise ValueError('Native grid comparison needs the same original kernel')
    if not current['capture_complete'] or not baseline['capture_complete']:
        raise ValueError('Original layout capture incomplete')
    if not pages or not old or not near(pages[0]['rectangle'][0], 0):
        raise ValueError('Missing original primary home grid')
    if not all(p['grid']['rows'] == 5 and p['grid']['valid'] for p in pages):
        reasons.append('NATIVE_ROOT_ROWS_NOT_APPLIED')
    if any(p['unsupported_cells'] for p in pages + old):
        reasons.append('UNKNOWN_ORIGINAL_ROOT_CELL_SIZE')
    current_regular, old_regular = sum(len(p['regular']) for p in pages), sum(len(p['regular']) for p in old)
    current_widgets, old_widgets = sum(len(p['widgets']) for p in pages), sum(len(p['widgets']) for p in old)
    if current_regular != old_regular:
        reasons.append('ORIGINAL_APPLICATION_VIEW_COUNT_CHANGED')
    if current_widgets != old_widgets:
        reasons.append('ORIGINAL_WIDGET_VIEW_COUNT_CHANGED')
    first = pages[0]
    clearances = []
    for widget in first['widgets']:
        x, y, width, height = widget['window_rectangle']
        labels = [dict(v, node=n) for n, v in current['views'].items()
                  if v['class'] == 'SBIconLegibilityLabelView' and
                  x <= v['window_rectangle'][0] + v['window_rectangle'][2] / 2 <= x + width and
                  y + 133 <= v['window_rectangle'][1] <= y + 185]
        if len(labels) != 1:
            reasons.append('WIDGET_ORIGINAL_LABEL_AMBIGUOUS')
            continue
        label = labels[0]
        lx, ly, lw, lh = label['window_rectangle']
        following = [cell for cell in first['regular'] if
                     cell['window_rectangle'][1] >= y + 133 and
                     cell['window_rectangle'][0] < lx + lw and lx < cell['window_rectangle'][0] + cell['window_rectangle'][2]]
        if not following:
            reasons.append('MISSING_FOLLOWING_ORIGINAL_ICON')
            continue
        next_cell = min(following, key=lambda cell: cell['window_rectangle'][1])
        gap = round(next_cell['window_rectangle'][1] - ly - lh, 3)
        clearances.append(dict(widget_node=widget['node'], label_node=label['node'],
                               next_icon_node=next_cell['node'], label_rectangle=label['window_rectangle'],
                               next_icon_rectangle=next_cell['window_rectangle'], clearance_points=gap))
        if gap < 8:
            reasons.append('ORIGINAL_WIDGET_LABEL_CLEARANCE_TOO_SMALL')
    if len(clearances) != len(first['widgets']) or not clearances:
        reasons.append('ORIGINAL_WIDGET_LABEL_INVENTORY_INCOMPLETE')
    for cell in first['regular'] + first['widgets']:
        x, y, width, height = cell['window_rectangle']
        if x < -.02 or y < -.02 or x + width > 360.02 or y + height > 640.02:
            reasons.append('ORIGINAL_PRIMARY_ICON_OUTSIDE_NATIVE_WINDOW')
    if current['counts']['refused'] or current['counts']['truncated']:
        reasons.append('ORIGINAL_LAYOUT_OBSERVER_INCOMPLETE')
    return dict(result='PASS' if not reasons else 'REVISE', reasons=sorted(set(reasons)),
                token=current['token'], raw_sha256=current['raw_sha256'],
                reference_token=baseline['token'], reference_raw_sha256=baseline['raw_sha256'],
                kernel_sha256=current['kernel_sha256'], original_current_controller_result=current['original_controller_result'],
                root_pages=[dict(node=p['node'], rectangle=p['rectangle'], rows=p['grid']['rows'],
                                 regular_icons=len(p['regular']), widgets=len(p['widgets'])) for p in pages],
                current_regular_view_count=current_regular, original_regular_view_count=old_regular,
                current_widget_view_count=current_widgets, original_widget_view_count=old_widgets,
                original_label_clearances=clearances, native_points=[360,640], native_pixels=[720,1280],
                original_application_identity_retention_NOT_verified=True,
                physical_panel_optical_FPS_wallpaper_peripherals_NOT_inferred=True)


def inspect(directory, baseline):
    current = inspect_layout(directory)
    original = inspect_layout(baseline)
    binding = current['geocompat_build_binding']
    if binding.get('native_root_grid_density_revision') != 1 or binding.get('native_root_grid_rows') != 5:
        raise ValueError('Missing exact compiled native grid profile')
    actual = json.loads((Path(directory) / 'actual-evidence.json').read_text())
    if not actual['actual_stage_verified'] or not actual['screen']['verified']:
        raise ValueError('Original GLES/scanout not independently verified')
    result = evaluate(current, original)
    result.update(source=str(Path(directory).resolve()), reference_source=str(Path(baseline).resolve()),
                  original_scanout=actual['screen'], geocompat_build_binding=binding,
                  independent_result_preserves_original_controller=True, device_operations=0)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--baseline', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    report = inspect(args.directory, args.baseline)
    with Path(args.output).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print(json.dumps({'result': report['result'], 'reasons': report['reasons'],
                      'root_pages': report['root_pages'], 'clearances': report['original_label_clearances']}))
