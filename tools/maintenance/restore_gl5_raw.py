"""Verify or restore the frozen Lilac gl5 sparse image to a new regular file.

The original sparse image is read-only. No device is opened, no existing output
is overwritten, and failed outputs are retained for inspection/recycling.
"""
import argparse
import hashlib
import os
from pathlib import Path
import stat
import struct
import sys
import zlib

SPARSE_SHA = 'e9bf1547b6a7e9a33b51c2749c643bc2eff0134c31cc3a63f9435e4cea1f9b29'
RAW_SHA = '4c90d65aa82c72aec8930feb2d9914b07750d879e6003dcae88884e74f63e631'
RAW_BYTES = 17179869184

def sha(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def decode(path, output):
    h = hashlib.sha256()
    crc = blocks = 0
    with path.open('rb') as stream:
        def read(n):
            data = stream.read(n)
            if len(data) != n:
                raise ValueError('Truncated sparse input')
            return data
        magic, major, minor, fh, ch, bs, total, chunks, image_crc = struct.unpack('<IHHHHIIII', read(28))
        if not (magic == 0xed26ff3a and major == 1 and minor == 0 and 28 <= fh <= 4096
                and 12 <= ch <= 4096 and 0 < bs <= 1048576 and bs % 4 == 0
                and total * bs == RAW_BYTES and 0 < chunks <= RAW_BYTES // 12):
            raise ValueError('Unexpected frozen gl5 sparse header')
        if read(fh - 28) != bytes(fh - 28):
            raise ValueError('Unsupported header extension')
        for _ in range(chunks):
            kind, reserved, count, size = struct.unpack('<HHII', read(12))
            if reserved or size < ch or count > total - blocks or read(ch - 12) != bytes(ch - 12):
                raise ValueError('Invalid chunk header')
            length, payload = count * bs, size - ch
            if kind == 0xcac1 and payload == length:
                pattern = None
            elif kind == 0xcac2 and payload == 4:
                pattern = read(4)
            elif kind == 0xcac3 and payload == 0:
                pattern = bytes(4)
            elif kind == 0xcac4 and count == 0 and payload == 4:
                if struct.unpack('<I', read(4))[0] != crc & 0xffffffff:
                    raise ValueError('Sparse CRC mismatch')
                continue
            else:
                raise ValueError('Invalid chunk type or size')
            while length:
                n = min(length, 8 << 20)
                data = read(n) if pattern is None else pattern * (n // 4)
                h.update(data)
                crc = zlib.crc32(data, crc)
                if output is not None:
                    if output.write(data) != n:
                        raise OSError('Short output write')
                length -= n
            blocks += count
        if blocks != total or stream.read(1) or (image_crc and image_crc != crc & 0xffffffff):
            raise ValueError('Sparse length or image CRC mismatch')
    if h.hexdigest() != RAW_SHA:
        raise ValueError('Decoded root fingerprint differs')

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--sparse', type=Path, required=True)
    mode = p.add_mutually_exclusive_group(required=True)
    mode.add_argument('--verify-only', action='store_true')
    mode.add_argument('--output', type=Path)
    a = p.parse_args()
    source = a.sparse.resolve(strict=True)
    if not stat.S_ISREG(source.stat().st_mode) or sha(source) != SPARSE_SHA:
        raise ValueError('Expected exact original Lilac gl5 regular file')
    if a.verify_only:
        decode(source, None)
    else:
        if sys.platform not in ('linux', 'darwin'):
            raise ValueError('Raw restoration uses macOS/Linux; verification is portable')
        target = a.output.absolute()
        if os.path.lexists(target) or target.parent.resolve().is_relative_to(Path('/dev')):
            raise ValueError('Output must be a new regular file outside /dev')
        with target.open('xb') as output:
            if not stat.S_ISREG(os.fstat(output.fileno()).st_mode):
                raise ValueError('Output is not a regular file')
            decode(source, output)
            output.flush()
            os.fsync(output.fileno())
        if target.stat().st_size != RAW_BYTES or sha(target) != RAW_SHA:
            raise ValueError('Output readback differs; unverified file retained')
    print(f'Verified {RAW_BYTES} decoded bytes, SHA256 {RAW_SHA}; no device operation')

if __name__ == '__main__':
    main()
