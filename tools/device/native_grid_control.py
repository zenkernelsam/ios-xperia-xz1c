"""Bind an explicit original/fitted native grid choice to its real boot env."""
import hashlib
from pathlib import Path


def specification(mode):
    if mode not in ('original', 'fitted'):
        raise ValueError('Unknown native grid mode')
    return {'revision': 1, 'mode': mode, 'value': 0 if mode == 'original' else 1,
            'source_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def apply(words, mode):
    binding = specification(mode)
    result, changed = [], 0
    for word in words:
        if not word.startswith('sweetenv='):
            result.append(word)
            continue
        segments = word[len('sweetenv='):].split(';')
        for index, segment in enumerate(segments):
            if not segment.startswith('SpringBoard:'):
                continue
            values = segment.split(':', 1)[1].split(',')
            if values.count('GEO_FIX=1') != 1 or any(v.startswith('GEO_GRID=') for v in values):
                raise ValueError('Native grid needs one original SpringBoard geometry env')
            segments[index] += ',GEO_GRID=' + str(binding['value'])
            changed += 1
        result.append('sweetenv=' + ';'.join(segments))
    if changed != 1:
        raise ValueError('Missing or repeated native grid env')
    return result, binding


def validate(record, package):
    geo = package.get('geocompat_build_binding', {})
    required = (record['trial_mode'] == 'gpu' and geo.get('revision') == 3 and
                record.get('display_geometry_binding', {}).get('profile') == 'xz1c-native-2x')
    actual = record.get('native_grid_control')
    if not required:
        if actual is not None:
            raise ValueError('Unbound native grid control on another trial')
        return None
    if actual is None or geo.get('native_root_grid_mode_control_revision') != 1:
        raise ValueError('Missing exact compiled native grid control')
    if actual != specification(actual.get('mode')):
        raise ValueError('Native grid control source/value differs')
    values = []
    for word in record['cmdline'].split():
        if word.startswith('sweetenv='):
            for segment in word[len('sweetenv='):].split(';'):
                if segment.startswith('SpringBoard:'):
                    values += [v for v in segment.split(':', 1)[1].split(',') if v.startswith('GEO_GRID=')]
    if values != ['GEO_GRID=' + str(actual['value'])]:
        raise ValueError('Native grid mode differs from actual SpringBoard env')
    return actual
