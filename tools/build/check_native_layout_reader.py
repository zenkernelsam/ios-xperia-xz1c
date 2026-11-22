"""Check bounded cache mapping and an analysis-only ARM64 wrapper."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

from hfs_sparse_reader import RawImage
from inspect_native_layout import CACHE_PATH, Cache, code_wrapper


def file_image(address, fill):
    data = bytearray(1536)
    data[:16] = b'dyld_v1   arm64\0'
    struct.pack_into('<II', data, 16, 512, 1)
    struct.pack_into('<QQQII', data, 512, address, 512, 1024, 5, 5)
    data[1024:] = bytes([fill]) * 512
    return RawImage(bytes(data))


class Files:
    def __init__(self, second=0x1200):
        self.files = {CACHE_PATH: file_image(0x1000, 0x31), CACHE_PATH + '.01': file_image(second, 0x72)}

    def open(self, name):
        return self.files[name]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ipsw', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    cache = Cache(Files())
    assert cache.read_at(0x11fc, 8) == b'1111rrrr'
    faults = {}
    for name, action in {'gap': lambda: cache.read_at(0x13ff, 2),
                         'negative': lambda: cache.read_at(-1, 8),
                         'overlap': lambda: Cache(Files(0x1100))}.items():
        try:
            action()
        except ValueError:
            faults[name] = True
        else:
            raise AssertionError(name)
    code = struct.pack('<II', 0xd28000a0, 0xd65f03c0)
    wrapper = code_wrapper(0x1c0000000, code)
    assert wrapper[4096:] == code and struct.unpack_from('<I', wrapper)[0] == 0xfeedfacf
    assert struct.unpack_from('<Q', wrapper, 32 + 24)[0] == 0x1c0000000
    path = args.output / 'analysis-fixture.macho'
    path.open('xb').write(wrapper)
    external = False
    if args.ipsw:
        result = subprocess.run([str(args.ipsw), 'macho', 'disass', str(path), '--vaddr', '0x1c0000000',
                                 '--count', '2', '--quiet', '--no-color'], capture_output=True, timeout=20)
        (args.output / 'analysis-fixture.txt').open('xb').write(result.stdout + result.stderr)
        assert result.returncode == 0, result.stderr
        assert b'mov' in result.stdout and b'ret' in result.stdout
        external = True
    root = Path(__file__).resolve().parents[2]
    paths = [Path(__file__), Path(__file__).with_name('inspect_native_layout.py')]
    report = dict(source_sha256={p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  original_cross_mapping_bytes_checked=8, faults_refused=faults,
                  wrapper_original_bytes_unchanged=True, native_ipsw_disassembly_verified=external,
                  fixture_is_synthetic_analysis_only=True, device_operations=0)
    (args.output / 'manifest.json').open('x').write(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
