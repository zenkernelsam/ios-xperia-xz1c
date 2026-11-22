"""Read exact 20H392 layout ABI/code through the immutable gl5 sparse image.

Only metadata/hashes are published. Small original code ranges and local
disassembly remain in build; no image is expanded and no device is operated.
"""
import argparse
from bisect import bisect_right
import hashlib
import json
import struct
import subprocess
import plistlib
from pathlib import Path

from a2s_lookup import A2S
from hfs_sparse_reader import AndroidSparseImage, HFS
from inspect_poster_routing import A2S_SHA, CACHE_SHA
from inspect_native_presentation import TOOL_SHA, ANCHORS as PRESENTATION_ANCHORS

ROOT = Path(__file__).resolve().parents[2]
SPARSE_SHA = 'e9bf1547b6a7e9a33b51c2749c643bc2eff0134c31cc3a63f9435e4cea1f9b29'
CACHE_PATH = '/System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64'
WANTED = {
    'SBIconListGridLayoutConfiguration': {'init', 'numberOfPortraitRows', 'numberOfPortraitColumns',
        'portraitLayoutInsets', 'listSizeForIconSpacingCalculation', 'iconImageInfo',
        'setNumberOfPortraitRows:', 'setListSizeForIconSpacingCalculation:'},
    'SBIconListView': {'iconSpacing', 'layoutMetrics', 'layoutMetricsForOrientation:',
        'layoutInsetsForOrientation:', 'layout', 'gridCellInfo', 'model'},
    'SBIconListGridLayout': {'initWithLayoutConfiguration:', 'numberOfRowsForOrientation:',
        'numberOfColumnsForOrientation:', 'layoutConfiguration'},
    'SBHDefaultIconListLayoutProvider': {'init', 'initWithScreenType:layoutOptions:', 'screenType',
        'makeLayoutForIconLocation:', 'layoutForIconLocation:',
        'configureGridSizeClassSizes:forScreenType:numberOfColumns:iconLocation:layoutOptions:'},
    'SBIconListModel': {'icons'},
    'SBLeafIcon': {'leafIdentifier', 'applicationBundleID', 'leafIdentifierAndApplicationBundleIDMatches'},
}
CLASS_WANTED = {'SBHDefaultIconListLayoutProvider': {'frameworkFallbackInstance', 'currentDeviceScreenType', 'defaultLayoutOptions'}}
METHOD_TABLES = {'SBLeafIcon': '__OBJC_$_INSTANCE_METHODS_SBLeafIcon(SBHTableViewIconLibraryAddtions)'}
REFERENCE_APPS = {'com.apple.facetime', 'com.apple.mobilecal', 'com.apple.mobileslideshow', 'com.apple.mobilemail',
                  'com.apple.mobilenotes', 'com.apple.reminders', 'com.apple.mobiletimer', 'com.apple.news',
                  'com.apple.tv', 'com.apple.podcasts', 'com.apple.AppStore', 'com.apple.Maps',
                  'com.apple.Health', 'com.apple.Passbook', 'com.apple.Preferences'}


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(8 << 20), b''):
            h.update(block)
    return h.hexdigest()


class Cache:
    def __init__(self, fs):
        self.maps = []
        self.files = []
        names = [p for p in fs.files if p == CACHE_PATH or
                 p.startswith(CACHE_PATH + '.') and not p.endswith('.symbols')]
        for name in names:
            fork = fs.open(name)
            header = fork.read_at(0, 512)
            if not header.startswith(b'dyld_v1   arm64'):
                raise ValueError('Unexpected original cache architecture')
            offset, count = struct.unpack_from('<II', header, 16)
            if not 0 < count <= 64 or offset + 32 * count > fork.size:
                raise ValueError('Invalid original cache mappings')
            table = fork.read_at(offset, count * 32)
            for address, length, physical, maximum, initial in struct.iter_unpack('<QQQII', table):
                if not length or physical + length > fork.size or initial & ~maximum:
                    raise ValueError('Invalid original cache extent')
                self.maps.append((address, length, physical, fork, name))
            self.files.append(dict(path=name, bytes=fork.size, header_sha256=hashlib.sha256(header).hexdigest()))
        self.maps.sort(key=lambda value: value[0])
        if not self.maps or any(a[0] + a[1] > b[0] for a, b in zip(self.maps, self.maps[1:])):
            raise ValueError('Overlapping or absent original cache mappings')
        self.starts = [x[0] for x in self.maps]

    def read_at(self, address, length):
        if address < 0 or length < 0:
            raise ValueError('Invalid original virtual range')
        parts = []
        while length:
            index = bisect_right(self.starts, address) - 1
            if index < 0:
                raise ValueError('Unmapped original address')
            start, span, offset, fork, _ = self.maps[index]
            count = min(length, start + span - address)
            if count <= 0:
                raise ValueError('Original address lies in mapping gap')
            parts.append(fork.read_at(offset + address - start, count))
            address += count
            length -= count
        return b''.join(parts)

    def string(self, address):
        data = self.read_at(address, 192)
        end = data.find(b'\0')
        if end < 0:
            raise ValueError('Unbounded original ABI string')
        return data[:end].decode('ascii')


def symbols_matching(mapping, needles):
    found = {}
    for needle in needles:
        cursor = mapping.strings_at
        while True:
            index = mapping.map.find(needle.encode('ascii'), cursor)
            if index < 0:
                break
            begin = max(mapping.strings_at, mapping.map.rfind(b'\0', mapping.strings_at, index) + 1)
            end = mapping.map.find(b'\0', index)
            if end < 0:
                raise ValueError('Invalid original symbol string')
            found[begin - mapping.strings_at] = mapping.map[begin:end].decode('utf-8')
            cursor = end + 1
    return {name: address for address, offset in struct.iter_unpack('<QI', mapping.map[12:mapping.strings_at])
            if (name := found.get(offset)) is not None}


def next_symbol(mapping, address):
    lo, hi = 0, mapping.count
    while lo < hi:
        mid = (lo + hi) // 2
        value = struct.unpack_from('<Q', mapping.map, 12 + mid * 12)[0]
        if value <= address:
            lo = mid + 1
        else:
            hi = mid
    return struct.unpack_from('<Q', mapping.map, 12 + lo * 12)[0] if lo < mapping.count else address + 1024


def code_wrapper(address, code):
    # An analysis-only header around original bytes; never a runnable payload.
    header = struct.pack('<8I', 0xfeedfacf, 0x100000c, 0, 2, 1, 152, 0, 0)
    segment = struct.pack('<II16sQQQQiiII', 0x19, 152, b'__TEXT', address, len(code),
                          4096, len(code), 5, 5, 1, 0)
    section = struct.pack('<16s16sQQIIIIIIII', b'__text', b'__TEXT', address, len(code),
                          4096, 2, 0, 0, 0x80000400, 0, 0, 0)
    return (header + segment + section).ljust(4096, b'\0') + code


def inspect(sparse, symbols, tool, output):
    sparse, symbols, tool, output = [Path(x).resolve() for x in (sparse, symbols, tool, output)]
    before = sparse.stat()
    if digest(sparse) != SPARSE_SHA or digest(symbols) != A2S_SHA or digest(tool) != TOOL_SHA:
        raise ValueError('Original analysis input fingerprint differs')
    if not output.is_relative_to(ROOT / 'build') or output.exists():
        raise ValueError('Native analysis needs a new build output')
    output.mkdir(parents=True)
    image = AndroidSparseImage(sparse)
    mapping = A2S(symbols)
    try:
        fs = HFS(image)
        base = fs.open(CACHE_PATH)
        if hashlib.sha256(base.read_at(0, base.size)).hexdigest() != CACHE_SHA:
            raise ValueError('Original dyld header fingerprint differs')
        cache = Cache(fs)
        verified_ranges = {}
        for name, (address, length, wanted) in PRESENTATION_ANCHORS.items():
            actual = hashlib.sha256(cache.read_at(address, length)).hexdigest()
            if actual != wanted:
                raise ValueError('Virtual cache range differs from original independent proof: ' + name)
            verified_ranges[name] = dict(address=hex(address), bytes=length, sha256=actual)
        names = symbols_matching(mapping, [METHOD_TABLES.get(x, '__OBJC_$_INSTANCE_METHODS_' + x) for x in WANTED] +
                                 ['__OBJC_$_CLASS_METHODS_' + x for x in CLASS_WANTED] +
                                 ['IconListLayoutProvider', 'WidgetMetricsForFamily', 'WidgetMetricsSpecification'])
        methods = []
        groups = [(owner, selectors, False) for owner, selectors in WANTED.items()] + [(owner, selectors, True) for owner, selectors in CLASS_WANTED.items()]
        for owner, selectors, class_method in groups:
            table_name = '__OBJC_$_CLASS_METHODS_' + owner if class_method else METHOD_TABLES.get(owner, '__OBJC_$_INSTANCE_METHODS_' + owner)
            table_at = names[table_name]
            flags, count = struct.unpack('<II', cache.read_at(table_at, 8))
            if flags != 0xc000000f or not 0 < count <= 4096:
                raise ValueError('Unexpected original layout method list')
            table = cache.read_at(table_at + 8, count * 12)
            seen = set()
            for index in range(count):
                entry = table_at + 8 + index * 12
                _, type_delta, imp_delta = struct.unpack_from('<iii', table, index * 12)
                imp = entry + 8 + imp_delta
                info = mapping.lookup(imp)
                if not info or info['symbol_offset'] != '0x0' or not info['symbol'].startswith(('+[' if class_method else '-[') + owner + ' '):
                    continue
                selector = info['symbol'].split(' ', 1)[1][:-1]
                if selector not in selectors:
                    continue
                if selector in seen:
                    raise ValueError('Duplicate original method')
                seen.add(selector)
                length = min(next_symbol(mapping, imp) - imp, 1024)
                length -= length % 4
                if length <= 0:
                    raise ValueError('Missing original method range')
                code = cache.read_at(imp, length)
                name = owner + ('-class-' if class_method else '-') + re_safe(selector)
                (output / (name + '.bin')).open('xb').write(code)
                wrapper = output / (name + '.macho')
                wrapper.open('xb').write(code_wrapper(imp, code))
                result = subprocess.run([str(tool), 'macho', 'disass', str(wrapper), '--vaddr', hex(imp),
                                         '--count', str(length // 4), '--quiet', '--no-color'],
                                        capture_output=True, timeout=55)
                (output / (name + '.txt')).open('xb').write(result.stdout + result.stderr)
                if result.returncode:
                    raise ValueError('Local native disassembly failed: ' + result.stderr[-2000:].decode(errors='replace'))
                methods.append(dict(owner=owner, selector=selector, types=cache.string(entry + 4 + type_delta),
                                    IMP=hex(imp), method_list=hex(table_at), exact_A2S_IMP_start=True,
                                    class_method=class_method,
                                    code_bytes=length, code_sha256=hashlib.sha256(code).hexdigest(),
                                    local_file=name + '.txt'))
            if seen != selectors:
                raise ValueError('Incomplete original ABI inventory: ' + owner + ': ' + str(selectors - seen))
        references = []
        for path in fs.files:
            if not path.endswith('.app/Info.plist') or not path.startswith(('/Applications/', '/System/Library/AppPlaceholders/', '/private/var/staged_system_apps/')):
                continue
            fork = fs.open(path)
            if not 0 < fork.size < 1 << 20:
                continue
            data = fork.read_at(0, fork.size)
            info = plistlib.loads(data)
            identifier = info.get('CFBundleIdentifier')
            if identifier in REFERENCE_APPS:
                references.append(dict(path=path, bundle_identifier=identifier,
                                       Info_plist_sha256=hashlib.sha256(data).hexdigest()))
        if {x['bundle_identifier'] for x in references} != REFERENCE_APPS:
            raise ValueError('Incomplete original application identity reference')
        current = sparse.stat()
        if (current.st_size, current.st_mtime_ns, current.st_ino) != (before.st_size, before.st_mtime_ns, before.st_ino):
            raise ValueError('Original input changed during observation')
        report = dict(system='iOS16.7.16/20H392', sparse_sha256=SPARSE_SHA, A2S_sha256=A2S_SHA,
                      cache_header_sha256=CACHE_SHA, ipsw_tool_sha256=TOOL_SHA,
                      original_methods=methods, provider_symbols={name: hex(address) for name, address in names.items()
                                                                if not name.startswith('__OBJC')},
                      cache_files=cache.files, catalog_files=len(fs.files), decoded_image_bytes=image.size,
                      independently_verified_original_ranges=verified_ranges,
                      source_sha256=digest(Path(__file__)), images_expanded_to_disk=0, image_mounts=0,
                      original_application_identity_references=references,
                      device_operations=0, original_Apple_bytes_not_for_Git=True,
                      static_methods_are_not_device_UI_acceptance=True)
        (output / 'report.json').open('x').write(json.dumps(report, indent=2) + '\n')
        return report
    finally:
        mapping.close()
        image.close()


def re_safe(value):
    return ''.join(x if x.isalnum() or x in '-_' else '_' for x in value)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('sparse', 'symbols', 'ipsw', 'output'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    result = inspect(args.sparse, args.symbols, args.ipsw, args.output)
    print(json.dumps({'methods': len(result['original_methods']), 'output': args.output,
                      'images_expanded_to_disk': 0, 'device_operations': 0}))
