"""Verify virtual ranges, original signed RAM files and HFS hard links."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

from hfs_sparse_reader import AndroidSparseImage, HFS, RawImage


def sparse(chunks, block=4096):
    total = sum(len(data) // block for kind, data in chunks)
    parts = [struct.pack('<IHHHHIIII', 0xed26ff3a, 1, 0, 28, 12, block, total, len(chunks), 0)]
    for kind, data in chunks:
        payload = data if kind == 0xcac1 else data[:4] if kind == 0xcac2 else b''
        parts.extend((struct.pack('<HHII', kind, 0, len(data) // block, 12 + len(payload)), payload))
    return b''.join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ram-package', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    raw = zlib.decompress((args.ram_package / 'glram.zblob').read_bytes()[40:])
    original = json.loads((args.ram_package / 'manifest.json').read_text())
    assert hashlib.sha256(raw).hexdigest() == original['raw_sha256']
    fixture_raw = bytes(range(256)) * 32
    fixture_fill = b'\x11\x22\x33\x44' * 2048
    fixture_hole = bytes(8192)
    expected = fixture_raw + fixture_fill + fixture_hole + fixture_raw
    mixed = AndroidSparseImage(sparse([(0xcac1, fixture_raw), (0xcac2, fixture_fill),
                                       (0xcac3, fixture_hole), (0xcac1, fixture_raw)]))
    ranges = [(0, len(expected)), (8191, 9), (16382, 11), (24575, 6), (9001, 7), (mixed.size, 0)]
    for offset, length in ranges:
        assert mixed.read_at(offset, length) == expected[offset:offset + length]
    refusals = {}
    for name, action in {
        'negative_range': lambda: mixed.read_at(-1, 1),
        'overrun_range': lambda: mixed.read_at(mixed.size - 1, 2),
        'truncated_sparse': lambda: AndroidSparseImage(sparse([(0xcac1, fixture_raw)])[:-1]),
        'invalid_header': lambda: AndroidSparseImage(bytes(28)),
        'invalid_HFS': lambda: HFS(RawImage(bytes(4096))),
    }.items():
        try:
            action()
        except (ValueError, struct.error):
            refusals[name] = True
        else:
            raise AssertionError(name)
    packages = [RawImage(raw), AndroidSparseImage(sparse([(0xcac1, raw)]))]
    results = []
    for reader in packages:
        fs = HFS(reader)
        for name in ('sweetgl.dylib', 'libglapi.0.dylib', 'georead.dylib', 'm', 'g'):
            fork = fs.open('/' + name)
            data = fork.read_at(0, fork.size)
            target = {'m': 'sweetgl.dylib', 'g': 'georead.dylib'}.get(name, name)
            assert data == (args.ram_package / target).read_bytes(), name
            results.append(dict(reader=type(reader).__name__, path=name, bytes=len(data),
                                sha256=hashlib.sha256(data).hexdigest()))
        reader.close()
    root = Path(__file__).resolve().parents[2]
    report = dict(source_sha256={str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in (Path(__file__), Path(__file__).with_name('hfs_sparse_reader.py'))},
                  original_ram_raw_sha256=original['raw_sha256'], original_files_and_aliases=results,
                  virtual_cross_chunk_ranges=len(ranges), faults_refused=refusals,
                  fixtures_only_in_memory=True, device_operations=0, images_expanded_to_disk=0)
    (out / 'manifest.json').open('x').write(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'original_reads': len(results), 'cross_chunk_ranges': len(ranges),
                      'faults_refused': refusals, 'output': str(out), 'device_operations': 0}))


if __name__ == '__main__':
    main()
