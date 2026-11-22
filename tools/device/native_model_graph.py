"""Validate the original root/dock/folder model ledger without creating records."""
import re
from collections import Counter


def fields(value):
    result = {}
    for word in value.split():
        if '=' not in word:
            raise ValueError('Incomplete model graph record')
        key, item = word.split('=', 1)
        if key in result or not item:
            raise ValueError('Duplicate or empty model graph field')
        result[key] = item
    return result


def identifier(value):
    return 0 < len(value) <= 64 and re.fullmatch('[A-Za-z0-9._-]+', value)


def decode(records, views, rectangles, counts):
    models, icons, links, summary = {}, {}, {}, None
    for kind, payload in records:
        value = fields(payload)
        if kind == 'MODEL':
            if set(value) != {'n', 'kind', 'parent', 'index', 'list', 'folder', 'count'}:
                raise ValueError('Incomplete original graph model')
            n = int(value['n'])
            item = {k: int(value[k]) for k in ('parent', 'index', 'list', 'count')}
            item.update(kind=value['kind'], folder=value['folder'])
            if n in models or not 1 <= n <= 2080 or item['kind'] not in ('root', 'dock', 'folder') or not 0 <= item['count'] <= 128:
                raise ValueError('Invalid or repeated original graph model')
            if not 0 <= item['index'] < 128 or not 0 <= item['list'] < 8:
                raise ValueError('Invalid original graph model position')
            if item['kind'] == 'folder':
                if n <= 2048 or not item['parent'] or not identifier(item['folder']) or item['folder'] == '-':
                    raise ValueError('Missing original folder model identity')
            elif item['parent'] or item['index'] or item['list'] or item['folder'] != '-':
                raise ValueError('Root/dock graph model has foreign parent')
            models[n] = item
        elif kind == 'ICON':
            if set(value) != {'n', 'index', 'class', 'application', 'id'}:
                raise ValueError('Incomplete original graph icon')
            n, index, app = int(value['n']), int(value['index']), int(value['application'])
            key = n, index
            if key in icons or not 0 <= index < 128 or app not in (0, 1) or not 0 < len(value['class']) <= 48:
                raise ValueError('Invalid or repeated original graph icon')
            if not identifier(value['id']) or app and value['id'] == '-':
                raise ValueError('Invalid original graph application identity')
            icons[key] = {'class': value['class'], 'application': bool(app), 'identifier': value['id']}
        elif kind == 'LINK':
            if set(value) != {'n', 'index', 'folder', 'lists'}:
                raise ValueError('Incomplete original folder link')
            key = int(value['n']), int(value['index'])
            count = int(value['lists'])
            if key in links or not 0 <= count <= 8 or not identifier(value['folder']) or value['folder'] == '-':
                raise ValueError('Invalid or repeated original folder link')
            links[key] = {'folder': value['folder'], 'lists': count}
        elif kind == 'MODELS':
            if summary is not None or set(value) != {'pages', 'icons', 'applications'}:
                raise ValueError('Incomplete or repeated original graph summary')
            summary = {k: int(v) for k, v in value.items()}
        else:
            raise ValueError('Unknown original graph record')
    if not models or len(models) > 32 or summary is None:
        raise ValueError('Missing original complete model graph')
    wanted_icons = {(n, index) for n, m in models.items() for index in range(m['count'])}
    if set(icons) != wanted_icons or summary != {'pages': len(models), 'icons': len(icons),
                                              'applications': sum(x['application'] for x in icons.values())}:
        raise ValueError('Missing original graph icon or inventory count')
    roots = {n for n, v in views.items() if v['class'] == 'SBIconListView' and
             abs(rectangles[n]['frame'][2] - 360) < .02 and abs(rectangles[n]['frame'][3] - 640) < .02}
    docks = {n for n, v in views.items() if v['class'] == 'SBDockIconListView'}
    if {n for n, m in models.items() if m['kind'] == 'root'} != roots or not roots:
        raise ValueError('Incomplete original root graph view inventory')
    if {n for n, m in models.items() if m['kind'] == 'dock'} != docks or len(docks) != 1:
        raise ValueError('Incomplete original dock graph view inventory')
    folder_slots = {key for key, icon in icons.items() if icon['class'] == 'SBFolderIcon'}
    if set(links) != folder_slots:
        raise ValueError('Missing original folder link inventory')
    expected_children = {(n, index, sub, link['folder']) for (n, index), link in links.items() for sub in range(link['lists'])}
    actual_children = {(m['parent'], m['index'], m['list'], m['folder']) for m in models.values() if m['kind'] == 'folder'}
    if actual_children != expected_children or len(actual_children) != sum(m['kind'] == 'folder' for m in models.values()):
        raise ValueError('Missing or repeated original nested folder model')
    for n, m in models.items():
        seen = set()
        while m['parent']:
            if n in seen or m['parent'] not in models or len(seen) >= 8:
                raise ValueError('Cyclic or foreign original model ancestry')
            seen.add(n)
            n = m['parent']
            m = models[n]
    result = {str(n): dict(m, icons=[icons[n, i] for i in range(m['count'])]) for n, m in models.items()}
    return result, summary, {str(n) + ':' + str(i): x for (n, i), x in links.items()}


def ledger(report):
    root_apps, all_apps, widgets, root_folders, docks, folders = Counter(), Counter(), Counter(), Counter(), [], {}
    for n, model in report['models'].items():
        apps = [x['identifier'] for x in model['icons'] if x['application']]
        all_apps.update(apps)
        if model['kind'] == 'root':
            root_apps.update(apps)
            widgets.update(x['class'] for x in model['icons'] if x['class'] == 'SBWidgetIcon')
            root_folders.update(report['model_links'][n + ':' + str(i)]['folder'] for i, x in enumerate(model['icons']) if x['class'] == 'SBFolderIcon')
        elif model['kind'] == 'dock':
            docks.append(tuple((x['class'], x['application'], x['identifier']) for x in model['icons']))
        else:
            key = model['folder'], model['list']
            if key in folders:
                raise ValueError('Repeated original folder ledger key')
            entries = []
            for i, x in enumerate(model['icons']):
                child = report['model_links'].get(n + ':' + str(i))
                entries.append((x['class'], x['application'], x['identifier'], child['folder'] if child else '-', child['lists'] if child else 0))
            folders[key] = tuple(entries)
    return dict(root_apps=root_apps, all_apps=all_apps, widgets=widgets, root_folders=root_folders, docks=docks, folders=folders)


def compare(current, baseline):
    if current['revision'] != 3 or baseline['revision'] != 3:
        raise ValueError('Complete model identity comparison requires original revision3 captures')
    if current['native_grid_value'] != 1 or baseline['native_grid_value'] != 0:
        raise ValueError('Qualified original/fitted model modes missing')
    for report in (current, baseline):
        if not report['capture_complete'] or not report['grid_fields_fully_observed']:
            raise ValueError('Original complete model fields not verified')
    if current['kernel_sha256'] != baseline['kernel_sha256'] or current['geocompat_build_binding'] != baseline['geocompat_build_binding']:
        raise ValueError('Original/fitted model comparison uses different kernel or helper')
    before, after = ledger(baseline), ledger(current)
    reasons = []
    for key in ('root_apps', 'all_apps', 'widgets', 'root_folders', 'docks', 'folders'):
        if before[key] != after[key]:
            reasons.append('ORIGINAL_MODEL_' + key.upper() + '_CHANGED')
    return dict(complete_original_model_identity_verified=not reasons, reasons=reasons,
                original_full_application_entries=sum(before['all_apps'].values()), fitted_full_application_entries=sum(after['all_apps'].values()),
                original_root_application_entries=sum(before['root_apps'].values()), fitted_root_application_entries=sum(after['root_apps'].values()),
                original_dock_model_count=len(before['docks']), fitted_dock_model_count=len(after['docks']),
                original_folder_list_count=len(before['folders']), fitted_folder_list_count=len(after['folders']),
                original_full_application_multiset=dict(before['all_apps']), fitted_full_application_multiset=dict(after['all_apps']),
                original_root_widget_classes=dict(before['widgets']), fitted_root_widget_classes=dict(after['widgets']),
                original_folder_identities=dict(before['root_folders']), fitted_folder_identities=dict(after['root_folders']),
                original_dock_entries=[list(x) for x in before['docks']], fitted_dock_entries=[list(x) for x in after['docks']],
                original_baseline_token=baseline['token'], original_baseline_raw_sha256=baseline['raw_sha256'],
                root_page_reflow_allowed=True, containers_and_full_identity_preserved=not reasons)
