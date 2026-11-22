"""Verify original view-tree edges and recompute label owners; never operate a device."""


def fields(payload):
    result = {}
    for word in payload.split():
        if '=' not in word:
            raise ValueError('Incomplete original ancestry record')
        key, value = word.split('=', 1)
        if key in result or not value:
            raise ValueError('Repeated or empty original ancestry field')
        result[key] = int(value)
    return result


def decode(records, views, counts):
    tree, owners = {}, {}
    for kind, payload in records:
        value = fields(payload)
        if kind == 'TREE':
            if set(value) != {'n', 'parent', 'depth', 'role'}:
                raise ValueError('Incomplete original tree edge')
            n, parent, depth, role = (value[k] for k in ('n', 'parent', 'depth', 'role'))
            if n != len(tree) + 1 or not 1 <= n <= 2048 or not 0 <= parent < n or not 0 <= depth <= 64 or role not in (0, 1, 2, 3):
                raise ValueError('Foreign, repeated or out-of-order original tree edge')
            if (not parent and depth != 0) or (parent and (parent not in tree or depth != tree[parent]['depth'] + 1)):
                raise ValueError('Original ancestry depth/parent mismatch')
            tree[n] = {'parent': parent, 'depth': depth, 'role': role}
        elif kind == 'OWNER':
            if set(value) != {'n', 'icon', 'list', 'hops', 'complete'}:
                raise ValueError('Incomplete original label owner')
            n = value.pop('n')
            if n in owners or n not in tree or tree[n]['role'] != 3 or value['complete'] != 1:
                raise ValueError('Foreign, repeated or unknown original label owner')
            if not 0 <= value['icon'] <= 2048 or not 0 <= value['list'] <= 2048 or not 0 <= value['hops'] <= 64:
                raise ValueError('Invalid original label owner bound')
            owners[n] = value
        else:
            raise ValueError('Unknown original ancestry record')
    if len(tree) != counts['visited'] or counts.get('tree_nodes') != len(tree):
        raise ValueError('Original complete tree inventory missing')
    labels = {n for n, node in tree.items() if node['role'] == 3}
    if set(owners) != labels or counts.get('owners') != len(labels):
        raise ValueError('Original label owner inventory missing')
    expected_roles = {'SBIconView': 1, 'SBIconListView': 2, 'SBDockIconListView': 2,
                      'SBIconLegibilityLabelView': 3, 'UIWindow': 0, 'UIView': 0, 'UILabel': 0}
    for n, view in views.items():
        if n not in tree or any(view[k] != tree[n][k] for k in ('parent', 'depth')):
            raise ValueError('Original view differs from original tree edge')
        if view['class'] in expected_roles and tree[n]['role'] != expected_roles[view['class']]:
            raise ValueError('Original typed view differs from original class role')
    for n, node in tree.items():
        if node['role'] and n not in views:
            raise ValueError('Original typed ancestor/label has no complete view record')
    # Native roles use the original class hierarchy, including opaque-named
    # subclasses. Exact known base classes still have to agree above.
    if not {n for n, view in views.items() if view['class'] == 'SBIconLegibilityLabelView'} <= labels:
        raise ValueError('Original label class inventory differs from typed tree')
    for n, owner in owners.items():
        parent, icon, listing, hops = tree[n]['parent'], 0, 0, 0
        while parent:
            node = tree[parent]
            hops += 1
            if not icon and node['role'] == 1:
                icon = parent
            if not listing and node['role'] == 2:
                listing = parent
            parent = node['parent']
        if owner != {'icon': icon, 'list': listing, 'hops': hops, 'complete': 1}:
            raise ValueError('Original owner differs from nearest typed ancestry')
    return ({str(n): value for n, value in tree.items()},
            {str(n): value for n, value in owners.items()})
