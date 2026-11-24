"""Bind original/fitted native grid choice to the boot env and exact helper."""
import hashlib
from pathlib import Path
import re


LEGACY_EXPLICIT_SOURCES = {
    '5eaa609b69c503c9fd4ebad15349c8079b05fb2c553d8b9565b285f12d7da0ab',
    '783c8393c0eadb0a831471b8b17ebd46b49aee4c0e010054085d5269a01f29a3',
}
HELPER_SOURCE = 'gpu/geocompat/geocompat.mm'
FITTED_DEFAULT_SOURCE_SHA = '9a67a77d1da071f24f6856f592a365f9ec7df17a754998ddbb3f28602a8d0091'
FITTED_DEFAULT_LIBRARY_SHA = 'ee7a3207f26af5e9106cbd98c74392153b11a2d345386964db131110a2a6153c'


def specification(mode):
    if mode not in ('original', 'fitted'):
        raise ValueError('Unknown native grid mode')
    return {'revision': 1, 'mode': mode, 'value': 0 if mode == 'original' else 1,
            'source_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}


def fitted_default(package):
    geo = package.get('geocompat_build_binding', {})
    if (geo.get('revision') != 4 or geo.get('native_root_grid_mode_control_revision') != 1
            or geo.get('library_sha256') != FITTED_DEFAULT_LIBRARY_SHA
            or geo.get('source_files', {}).get(HELPER_SOURCE) != FITTED_DEFAULT_SOURCE_SHA
            or package.get('files', {}).get('georead.dylib', {}).get('sha256') != FITTED_DEFAULT_LIBRARY_SHA):
        return None
    return dict(specification('fitted'), revision=2, env='helper-default',
                helper_sha256=FITTED_DEFAULT_LIBRARY_SHA, helper_source_sha256=FITTED_DEFAULT_SOURCE_SHA)


def springboard_environment(words):
    env = [(index, word[len('sweetenv='):].split(';')) for index, word in enumerate(words)
           if word.startswith('sweetenv=')]
    if len(env) != 1:
        raise ValueError('Expected one native grid environment')
    word_index, segments = env[0]
    springboard = [index for index, segment in enumerate(segments) if segment.startswith('SpringBoard:')]
    if len(springboard) != 1:
        raise ValueError('Missing or repeated SpringBoard environment')
    segment_index = springboard[0]
    values = segments[segment_index].split(':', 1)[1].split(',')
    if [value for value in values if value.split('=', 1)[0] == 'GEO_FIX'] != ['GEO_FIX=1']:
        raise ValueError('Native grid needs one enabled SpringBoard geometry env')
    return word_index, segments, segment_index, values


def grid_values(values):
    return [value for value in values if value.split('=', 1)[0] == 'GEO_GRID']


def apply(words, mode):
    binding = specification(mode)
    word_index, segments, segment_index, values = springboard_environment(words)
    if grid_values(values):
        raise ValueError('Native grid already has an environment value')
    segments[segment_index] += ',GEO_GRID=' + str(binding['value'])
    result = list(words)
    result[word_index] = 'sweetenv=' + ';'.join(segments)
    return result, binding


def compact_fitted(words, control, package):
    binding = fitted_default(package)
    if binding is None or control != specification('fitted'):
        return None
    word_index, segments, segment_index, values = springboard_environment(words)
    if grid_values(values) != ['GEO_GRID=1']:
        raise ValueError('Expected exactly one explicit fitted value before compaction')
    values.remove('GEO_GRID=1')
    segments[segment_index] = 'SpringBoard:' + ','.join(values)
    result = list(words)
    result[word_index] = 'sweetenv=' + ';'.join(segments)
    return result, binding


def validate(record, package):
    geo = package.get('geocompat_build_binding', {})
    required = (record['trial_mode'] == 'gpu' and geo.get('revision') in (3, 4) and
                record.get('display_geometry_binding', {}).get('profile') == 'xz1c-native-2x')
    actual = record.get('native_grid_control')
    if not required:
        if actual is not None:
            raise ValueError('Unbound native grid control on another trial')
        return None
    if actual is None or geo.get('native_root_grid_mode_control_revision') != 1:
        raise ValueError('Missing exact compiled native grid control')
    _, _, _, values = springboard_environment(record['cmdline'].split())
    if actual.get('revision') == 2:
        if actual != fitted_default(package) or grid_values(values):
            raise ValueError('Native grid default differs from the exact helper or actual SpringBoard env')
        if [value for value in values if value.split('=', 1)[0] == 'DYLD_INSERT_LIBRARIES'] != [
            'DYLD_INSERT_LIBRARIES=/dev/sglram/g'
        ]:
            raise ValueError('Default grid mode needs the exact SpringBoard helper injection')
    else:
        expected = specification(actual.get('mode'))
        if actual.get('source_sha256') in LEGACY_EXPLICIT_SOURCES:
            expected['source_sha256'] = actual['source_sha256']
        if actual != expected or grid_values(values) != ['GEO_GRID=' + str(expected['value'])]:
            raise ValueError('Explicit native grid mode differs from its source or actual SpringBoard env')
    return actual


def verify_default_runtime(scope):
    markers = re.findall(r'(?m)^\|SpringBoard\| SWEET GEOGRID MODE value=([^\r\n]*)', scope)
    if not markers or any(marker != '1' for marker in markers):
        raise ValueError('SpringBoard did not confirm the fitted helper default in this boot')
