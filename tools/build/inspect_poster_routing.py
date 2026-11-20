"""Read original 20H392 animation and canvas choices; never operate a phone.

The mounted root, A2S and ipsw CLI are supplied local inputs. Only hashes,
method metadata and conclusions enter the report; disassembly stays in build.
"""
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import re
import shutil
import struct
import subprocess
import sys

from a2s_lookup import A2S

ROOT = Path(__file__).resolve().parents[2]
A2S_SHA = '5bf04681af1819e2379319942a97be635100d6ca3844c5a22e372ce78a83b83d'
CACHE_SHA = '76874e8375252ee643011ff8063f0cce12355f05cf5f9897a34e5679667a65aa'
AEGIR_SHA = 'a4abba1e22350a5c577dc3ce551da90ab42606d872509cd50859ef63751454a7'
AEGIR_PATH = 'System/Library/CoreServices/AegirProxyApp.app/PlugIns/AegirPoster.appex/AegirPoster'
METHODS = {
    0x1ee94ea00: ('activeMode', 'v16@0:8'),
    0x1ee94ea60: ('deactiveMode', 'v16@0:8'),
    0x1ee94eac8: ('interactiveMode', 'v16@0:8'),
    0x1ee94eb1c: ('supplementalMode', 'v16@0:8'),
    0x1ee94e898: ('animateTransitionToMode:', 'v24@0:8q16'),
    0x1ee94eb70: ('applyMode:', 'v24@0:8q16'),
}
METHOD_LIST = 0x1ee971ff8
ANCHORS = {
    'vista-update': (0x1ee94797c, 10, '7e677a7a756c7fc011cba6d4e73d983463aedb6cda31044351f51e2e2d83e6c6'),
    'vista-display-init': (0x1ee9471dc, 6, '8499c8077a8769038156344392ac9f0f39dc998c65799439ffbb66bedaaf547b'),
    'vista-start': (0x1ee94794c, 6, '4efb554e71cf7cc1c576e3a2aae129d84c41f382b9545e54c8cdf226989143cd'),
    'vista-resume': (0x1ee94788c, 24, '98319b660354d6a22977a3962678a210163b8ecfe922bd44b18d1a3f3e1a0db1'),
    'controller-active': (0x1ee94ea00, 24, '6c31bd4e57d61eda55bddf2ab89e0486c6115e4df26f495671b89bdbe335a5e0'),
    'controller-dispatch': (0x1ee94eb70, 44, 'c2b1667b8d766ee1e20a03a37690c6fc6341bb6371647938d8169fef515e6caf'),
    'canvas-options': (0x1c1733548, 40, '50a96f32bb4b3e0f53a2ec8414dd7982e1ab27a1847eadfff6d3db4006d82106'),
    'canvas-msaa-count': (0x1c1738330, 6, '9a2b9619316801bc7b1a0e979b1f987fdae59c1e19bc3409bcb1dce780747098'),
    'canvas-depth-descriptor': (0x1c1733da0, 84, '4c0bc6af8bc88d950257c80726f444ec6e5c11a7fba03acf42eff9de459a94cf'),
}


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(8 << 20), b''):
            value.update(block)
    return value.hexdigest()


def macho_string(data, address):
    assert data[:4] == b'\xcf\xfa\xed\xfe'
    at = 32
    for _ in range(struct.unpack_from('<I', data, 16)[0]):
        command, size = struct.unpack_from('<II', data, at)
        assert size >= 8 and at + size <= len(data)
        if command == 0x19:
            _, vm, _, offset, span = struct.unpack_from('<16sQQQQ', data, at + 8)
            if vm <= address < vm + span:
                begin = offset + address - vm
                assert begin < len(data) and offset + span <= len(data)
                end = data.find(b'\0', begin, min(offset + span, begin + 192))
                assert end >= begin, 'Unbounded original method type'
                return data[begin:end].decode('ascii')
        at += size
    raise ValueError('Original method type lies outside its actual image')


def method_metadata(table, image, symbols):
    flags, count = struct.unpack_from('<II', table)
    assert flags == 0xc000000f and count == 81 and len(table) >= 8 + count * 12
    records = []
    for index in range(count):
        entry = METHOD_LIST + 8 + index * 12
        _, types, imp = struct.unpack_from('<iii', table, 8 + index * 12)
        actual = entry + 8 + imp
        if actual not in METHODS:
            continue
        selector, expected = METHODS[actual]
        encoded = macho_string(image, entry + 4 + types)
        symbol = symbols.lookup(actual)
        assert encoded == expected and symbol['symbol_offset'] == '0x0'
        assert symbol['symbol'] == '-[NUNIAstronomyVistaController ' + selector + ']'
        records.append(dict(owner='NUNIAstronomyVistaController', selector=selector,
            types=encoded, imp=hex(actual), entry_index=index, method_list=hex(METHOD_LIST),
            type_address=hex(entry + 4 + types), exact_A2S_IMP_start_verified=True))
    assert len(records) == len(METHODS)
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True, help='Existing read-only mounted gl5 root')
    parser.add_argument('--symbols', type=Path, required=True)
    parser.add_argument('--ipsw', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sys.platform == 'darwin', 'Original cache reader runs locally on Mac'
    mounted = args.root.resolve(strict=True)
    cache = mounted / 'System/Library/Caches/com.apple.dyld/dyld_shared_cache_arm64'
    aegir = mounted / AEGIR_PATH
    symbols = args.symbols.resolve(strict=True)
    tool = args.ipsw.resolve(strict=True)
    before = {str(p): digest(p) for p in (cache, aegir, symbols, tool)}
    assert before[str(cache)] == CACHE_SHA and before[str(aegir)] == AEGIR_SHA and before[str(symbols)] == A2S_SHA
    info = subprocess.run(['diskutil', 'info', '-plist', str(mounted)], capture_output=True, check=True, timeout=10)
    volume = plistlib.loads(info.stdout)
    assert volume['MountPoint'] == str(mounted) and volume['WritableVolume'] is False and volume['WritableMedia'] is False
    out = args.output.resolve()
    assert out.is_relative_to(ROOT / 'build') and not out.exists()
    out.mkdir(parents=True)
    local_symbols = out / 'symbols.a2s'
    shutil.copyfile(symbols, local_symbols)

    def run(name, arguments):
        result = subprocess.run([str(tool), *arguments, '--no-color'], capture_output=True, timeout=55)
        data = result.stdout + result.stderr
        assert len(data) < 4 << 20, 'Unexpected original analysis output'
        (out / (name + '.txt')).open('xb').write(data)
        assert result.returncode == 0, data[-2000:].decode(errors='replace')
        return result.stdout.decode('utf-8')

    def cache_code(name, address, count):
        expected_address, expected_count, wanted = ANCHORS[name]
        assert (address, count) == (expected_address, expected_count)
        binary = out / (name + '.bin')
        run(name + '-bytes', ['dyld', 'dump', str(cache), hex(address), '--size', str(count * 4), '--output', str(binary)])
        assert binary.stat().st_size == count * 4 and digest(binary) == wanted, 'Exact original code anchor differs: ' + name
        return run(name, ['dyld', 'disass', str(cache), '--vaddr', hex(address), '--count', str(count), '--cache', str(local_symbols)])

    animation = cache_code('vista-update', 0x1ee94797c, 10)
    init = cache_code('vista-display-init', 0x1ee9471dc, 6)
    start = cache_code('vista-start', 0x1ee94794c, 6)
    resume = cache_code('vista-resume', 0x1ee94788c, 24)
    active = cache_code('controller-active', 0x1ee94ea00, 24)
    dispatch = cache_code('controller-dispatch', 0x1ee94eb70, 44)
    options = cache_code('canvas-options', 0x1c1733548, 40)
    samples = cache_code('canvas-msaa-count', 0x1c1738330, 6)
    depth = cache_code('canvas-depth-descriptor', 0x1c1733da0, 84)
    wake = run('aegir-wake', ['macho', 'disass', str(aegir), '--vaddr', '0x100018d48', '--count', '44', '--cache', str(out / 'aegir.a2s')])
    assert re.search(r'tbz\s+w8, #0x2', animation) and re.search(r'tbnz\s+w8, #0x1', animation)
    assert '_isDisplayOn' in init and re.search(r'orr\s+w9, w9, #0x2', init)
    assert '_isAnimating' in start and re.search(r'orr\s+w9, w9, #0x4', start)
    assert re.search(r'mov\s+w2, #0', resume) and '_objc_msgSend$setPaused:' in resume
    assert '_objc_msgSend$startAnimation' in active and '_objc_msgSend$activeMode' in dispatch
    assert re.search(r'ands\s+x8, x8, #0x8', options) and '_objc_msgSend$_msaaCount' in options
    assert re.search(r'mov\s+x0, #0x4', samples)
    assert re.search(r'mov\s+x2, #0xfc', depth) and re.search(r'mov\s+x2, #0x3', depth)
    assert '_objc_msgSend$setStorageMode:' in depth and '_objc_msgSend$setUsage:' in depth
    assert 'sel_backlightProgress' in wake and 'sel_linearBacklightProgress' in wake
    for name, address, size in [('method-list', METHOD_LIST, 2048), ('mode-jumps', 0x1ee973724, 4)]:
        run(name, ['dyld', 'dump', str(cache), hex(address), '--size', str(size), '--output', str(out / (name + '.bin'))])
    jump_table = (out / 'mode-jumps.bin').read_bytes()
    assert jump_table == bytes((0, 3, 6, 16))
    run('nano-extract', ['dyld', 'macho', str(cache), 'NanoUniverse', '--extract', '--output', str(out / 'nano')])
    mapping = A2S(symbols)
    try:
        methods = method_metadata((out / 'method-list.bin').read_bytes(), (out / 'nano/NanoUniverse').read_bytes(), mapping)
    finally:
        mapping.close()
    assert before == {str(p): digest(p) for p in (cache, aegir, symbols, tool)}, 'Original analysis input changed'
    report = dict(system='iOS16.7.16/20H392', A2S_sha256=A2S_SHA, cache_header_sha256=CACHE_SHA,
        Aegir_sha256=AEGIR_SHA, ipsw_tool_sha256=before[str(tool)], root_readonly_verified=True,
        controller_methods=methods, original_mode_dispatch={'0': 'activeMode', '1': 'deactiveMode', '2': 'interactiveMode', '3': 'supplementalMode'},
        mode_dispatch_raw_jump_table_sha256=digest(out / 'mode-jumps.bin'),
        Vista_init_sets_display_on=True, Vista_animation_needs_display_and_animation_bits=True,
        active_mode_calls_original_startAnimation=True, original_start_clears_quad_paused=True,
        ClockKitUI_MSAA_option_bit=8, ClockKitUI_MSAA_sample_count_when_requested=4,
        ClockKitUI_depth_option_bit=1, ClockKitUI_depth_format=252,
        ClockKitUI_depth_storage_mode=3, ClockKitUI_depth_usage=4,
        Aegir_reads_original_backlight_and_linear_backlight=True,
        metadata_and_static_calls_are_not_runtime_acceptance=True,
        exact_code_anchors_verified={name: dict(address=hex(address), bytes=count * 4, sha256=sha)
            for name, (address, count, sha) in ANCHORS.items()},
        original_Apple_bytes_not_for_Git=True, phone_downloads=0, phone_boots=0, phone_resets=0,
        tool_source_sha256=digest(Path(__file__)),
        local_analysis_hashes={p.name: digest(p) for p in out.iterdir() if p.is_file() and p.suffix in ('.txt', '.bin')})
    (out / 'manifest.json').open('xb').write((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps(report))


if __name__ == '__main__':
    main()
