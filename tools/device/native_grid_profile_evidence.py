"""Verify original native grid reflow and label clearance; never operate a phone."""
import argparse
import json
from pathlib import Path

from geometry_layout_evidence import inspect as inspect_layout
from native_model_graph import compare as compare_complete_models

REFERENCE_APPS = {'com.apple.facetime', 'com.apple.mobilecal', 'com.apple.mobileslideshow', 'com.apple.mobilemail',
                  'com.apple.mobilenotes', 'com.apple.reminders', 'com.apple.mobiletimer', 'com.apple.news',
                  'com.apple.tv', 'com.apple.podcasts', 'com.apple.AppStore', 'com.apple.Maps',
                  'com.apple.Health', 'com.apple.Passbook', 'com.apple.Preferences'}


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


def evaluate(current, baseline, expected_applications=None):
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
    identities_verified = False
    complete_model_comparison = None
    model_page_counts = {}
    if current['revision'] in (3, 4):
        complete_model_comparison = compare_complete_models(current, baseline)
        reasons += complete_model_comparison['reasons']
        identities_verified = complete_model_comparison['complete_original_model_identity_verified']
        for p in pages:
            model = current['models'].get(p['node'])
            if model:
                app_count = sum(x['application'] for x in model['icons'])
                model_page_counts[p['node']] = app_count
                if near(p['rectangle'][0], 0) and len(p['regular']) != app_count:
                    reasons.append('VISIBLE_PRIMARY_APPLICATION_VIEWS_DIFFER_FROM_MODEL')
    elif current['revision'] == 2:
        if expected_applications is None or set(expected_applications) != REFERENCE_APPS:
            raise ValueError('Missing exact original application identity reference')
        model_ids = [x['identifier'] for p in current['models'].values() for x in p['icons'] if x['application']]
        if set(model_ids) > set(expected_applications) and old_regular == len(expected_applications):
            reasons.append('ORIGINAL_REFERENCE_ONLY_COVERS_PRIMARY_PAGE')
        elif set(model_ids) != set(expected_applications) or len(model_ids) != len(expected_applications) or old_regular != len(expected_applications):
            reasons.append('ORIGINAL_APPLICATION_MODEL_IDENTITIES_CHANGED')
        else:
            identities_verified = True
        if set(current['models']) != {p['node'] for p in pages}:
            reasons.append('ORIGINAL_ROOT_MODEL_PAGE_INVENTORY_CHANGED')
        for p in pages:
            model = current['models'].get(p['node'])
            if model:
                app_count = sum(x['application'] for x in model['icons'])
                model_page_counts[p['node']] = app_count
                if near(p['rectangle'][0], 0) and len(p['regular']) != app_count:
                    reasons.append('VISIBLE_PRIMARY_APPLICATION_VIEWS_DIFFER_FROM_MODEL')
        model_widgets = sum(x['class'] == 'SBWidgetIcon' for p in current['models'].values() for x in p['icons'])
        if model_widgets != old_widgets:
            reasons.append('ORIGINAL_WIDGET_MODEL_COUNT_CHANGED')
        if any(x['class'] == 'SBFolderIcon' for p in current['models'].values() for x in p['icons']):
            reasons.append('ORIGINAL_FOLDER_MODEL_NOT_OBSERVED')
    elif current_regular != old_regular:
        reasons.append('ORIGINAL_APPLICATION_VIEW_COUNT_CHANGED')
    if current['revision'] not in (3, 4) and current_widgets != old_widgets:
        reasons.append('ORIGINAL_WIDGET_VIEW_COUNT_CHANGED')
    first = pages[0]
    clearances = []
    for widget in first['widgets']:
        x, y, width, height = widget['window_rectangle']
        if current['revision'] == 4:
            if not current.get('original_label_ownership_complete') or not baseline.get('original_label_ownership_complete'):
                raise ValueError('Original label ancestry incomplete')
            labels = [dict(v, node=n) for n, v in current['views'].items()
                      if current['view_tree'].get(n, {}).get('role') == 3 and
                      current['label_owners'].get(n, {}).get('icon') == int(widget['node']) and
                      current['label_owners'].get(n, {}).get('list') == int(first['node'])]
        else:
            labels = [dict(v, node=n) for n, v in current['views'].items()
                      if v['class'] == 'SBIconLegibilityLabelView' and
                      x <= v['window_rectangle'][0] + v['window_rectangle'][2] / 2 <= x + width and
                      y + 133 <= v['window_rectangle'][1] <= y + 185]
        if len(labels) != 1:
            reasons.append('WIDGET_ORIGINAL_LABEL_AMBIGUOUS')
            continue
        label = labels[0]
        lx, ly, lw, lh = label['window_rectangle']
        if current['revision'] == 4 and not (lw > 0 and lh > 0 and
                x <= lx + lw / 2 <= x + width and y + 133 <= ly <= y + 185 and
                lx >= -.02 and ly >= -.02 and lx + lw <= 360.02 and ly + lh <= 640.02):
            reasons.append('ORIGINAL_OWNED_WIDGET_LABEL_GEOMETRY_INVALID')
            continue
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
    if current['counts']['refused'] or current['counts']['truncated'] or current['counts'].get('write_failures', 0):
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
                original_application_model_identity_retention_verified=identities_verified,
                original_application_identity_retention_NOT_verified=not identities_verified,
                original_model_application_count_by_page=model_page_counts,
                complete_original_model_comparison=complete_model_comparison,
                original_label_ancestry_verified=current['revision'] == 4 and current.get('original_label_ownership_complete', False),
                application_launch_functionality_NOT_verified=True,
                physical_panel_optical_FPS_wallpaper_peripherals_NOT_inferred=True)


def inspect(directory, baseline, reference=None):
    current = inspect_layout(directory)
    original = inspect_layout(baseline)
    binding = current['geocompat_build_binding']
    if binding.get('native_root_grid_density_revision') != 1 or binding.get('native_root_grid_rows') != 5:
        raise ValueError('Missing exact compiled native grid profile')
    actual = json.loads((Path(directory) / 'actual-evidence.json').read_text())
    if not actual['actual_stage_verified'] or not actual['screen']['verified']:
        raise ValueError('Original GLES/scanout not independently verified')
    identities = None
    if current['revision'] == 2:
        if reference is None:
            raise ValueError('New model capture requires the original gl5 identity proof')
        proof = json.loads(Path(reference).read_text())
        if proof.get('sparse_sha256') != 'e9bf1547b6a7e9a33b51c2749c643bc2eff0134c31cc3a63f9435e4cea1f9b29':
            raise ValueError('Original application reference image differs')
        identities = {x['bundle_identifier'] for x in proof['original_application_identity_references']}
        if identities != REFERENCE_APPS:
            raise ValueError('Incomplete original gl5 application reference')
    result = evaluate(current, original, identities)
    result.update(source=str(Path(directory).resolve()), reference_source=str(Path(baseline).resolve()),
                  original_scanout=actual['screen'], geocompat_build_binding=binding,
                  independent_result_preserves_original_controller=True, device_operations=0)
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', required=True)
    parser.add_argument('--baseline', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--reference')
    args = parser.parse_args()
    report = inspect(args.directory, args.baseline, args.reference)
    with Path(args.output).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(report, stream, ensure_ascii=False, indent=2)
        stream.write('\n')
    print(json.dumps({'result': report['result'], 'reasons': report['reasons'],
                      'root_pages': report['root_pages'], 'clearances': report['original_label_clearances']}))
