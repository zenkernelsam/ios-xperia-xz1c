"""Read HFS+/HFSX files directly from immutable raw or Android sparse inputs.

All offsets are checked. The reader never mounts, expands, changes or removes
an image. Native code investigation can read small ranges from a large input.
"""
from bisect import bisect_right
import io
from pathlib import Path
import struct


class RawImage:
    def __init__(self, source):
        self.stream = io.BytesIO(source) if isinstance(source, bytes) else Path(source).open('rb')
        self.stream.seek(0, 2)
        self.size = self.stream.tell()

    def read_at(self, offset, length):
        if offset < 0 or length < 0 or offset + length > self.size:
            raise ValueError('Raw image range outside input')
        self.stream.seek(offset)
        value = self.stream.read(length)
        if len(value) != length:
            raise ValueError('Truncated raw range')
        return value

    def close(self):
        self.stream.close()


class AndroidSparseImage(RawImage):
    def __init__(self, source):
        super().__init__(source)
        physical = self.size
        magic, major, minor, fh, ch, block, total, chunks, crc = struct.unpack('<IHHHHIIII', super().read_at(0, 28))
        if not (magic == 0xed26ff3a and major == 1 and minor == 0 and 28 <= fh <= 4096 and
                12 <= ch <= 4096 and 0 < block <= 1048576 and block % 4 == 0 and 0 < chunks <= 1000000):
            raise ValueError('Unsupported Android sparse header')
        if super().read_at(28, fh - 28) != bytes(fh - 28):
            raise ValueError('Unsupported sparse header extension')
        self.regions, self.starts = [], []
        at, cursor = 0, fh
        for _ in range(chunks):
            kind, reserved, blocks, size = struct.unpack('<HHII', super().read_at(cursor, 12))
            if reserved or size < ch or cursor + size > physical or at + blocks * block > total * block:
                raise ValueError('Invalid sparse chunk extent')
            if super().read_at(cursor + 12, ch - 12) != bytes(ch - 12):
                raise ValueError('Unsupported sparse chunk extension')
            length, payload, data_at = blocks * block, size - ch, cursor + ch
            pattern = None
            if kind == 0xcac1 and payload == length and length:
                pass
            elif kind == 0xcac2 and payload == 4 and length:
                pattern = super().read_at(data_at, 4)
            elif kind == 0xcac3 and payload == 0 and length:
                pattern = bytes(4)
            elif kind == 0xcac4 and blocks == 0 and payload == 4:
                cursor += size
                continue
            else:
                raise ValueError('Unsupported sparse chunk')
            self.starts.append(at)
            self.regions.append((at, length, data_at, pattern))
            at += length
            cursor += size
        if at != total * block or cursor != physical:
            raise ValueError('Incomplete sparse input')
        self.physical_size, self.size, self.image_crc = physical, at, crc

    def read_at(self, offset, length):
        if offset < 0 or length < 0 or offset + length > self.size:
            raise ValueError('Sparse range outside decoded input')
        parts = []
        while length:
            index = bisect_right(self.starts, offset) - 1
            start, span, physical, pattern = self.regions[index]
            count = min(length, start + span - offset)
            if count <= 0:
                raise ValueError('Invalid sparse range coverage')
            if pattern is None:
                self.stream.seek(physical + offset - start)
                value = self.stream.read(count)
                if len(value) != count:
                    raise ValueError('Truncated sparse data')
            else:
                shift = (offset - start) % 4
                value = (pattern * ((shift + count + 3) // 4))[shift:shift + count]
            parts.append(value)
            offset += count
            length -= count
        return b''.join(parts)


class Fork:
    def __init__(self, fs, descriptor, file_id, overflow=True):
        if len(descriptor) != 80:
            raise ValueError('Incomplete HFS fork')
        self.fs, self.file_id = fs, file_id
        self.size, _, total = struct.unpack_from('>QII', descriptor)
        self.extents = [(a, n) for a, n in struct.iter_unpack('>II', descriptor[16:]) if n]
        covered = sum(n for a, n in self.extents)
        while covered < total:
            if not overflow:
                raise ValueError('HFS system fork needs unsupported overflow')
            extra = fs.overflow.get((file_id, 0, covered))
            if not extra:
                raise ValueError('Missing HFS overflow extent')
            self.extents.extend(extra)
            next_covered = covered + sum(n for a, n in extra)
            if next_covered <= covered or next_covered > total:
                raise ValueError('Invalid HFS overflow coverage')
            covered = next_covered
        if covered != total or self.size > total * fs.block or any(a + n > fs.blocks for a, n in self.extents):
            raise ValueError('Invalid HFS fork allocation')

    def read_at(self, offset, length):
        if offset < 0 or length < 0 or offset + length > self.size:
            raise ValueError('HFS file range outside fork')
        parts, logical = [], 0
        for start, blocks in self.extents:
            span = blocks * self.fs.block
            if length and logical <= offset < logical + span:
                count = min(length, logical + span - offset)
                parts.append(self.fs.image.read_at(start * self.fs.block + offset - logical, count))
                offset += count
                length -= count
            logical += span
        if length:
            raise ValueError('Incomplete HFS extent read')
        return b''.join(parts)


def leaf_records(fork):
    head = fork.read_at(0, 64)
    if struct.unpack_from('>b', head, 8)[0] != 1:
        raise ValueError('Missing HFS B-tree header')
    first, last = struct.unpack_from('>II', head, 24)
    node_size = struct.unpack_from('>H', head, 32)[0]
    if not (512 <= node_size <= 32768 and node_size & (node_size - 1) == 0):
        raise ValueError('Invalid HFS B-tree node size')
    if (first == 0) != (last == 0):
        raise ValueError('Incomplete HFS leaf boundaries')
    seen, number = set(), first
    while number:
        if number in seen or (number + 1) * node_size > fork.size:
            raise ValueError('Cyclic or foreign HFS leaf')
        seen.add(number)
        node = fork.read_at(number * node_size, node_size)
        next_node, _, kind, height, count, _ = struct.unpack_from('>IIbBHH', node)
        if kind != -1 or height != 1 or count > (node_size - 14) // 4:
            raise ValueError('Invalid HFS leaf node')
        offsets = [struct.unpack_from('>H', node, node_size - 2 * (i + 1))[0] for i in range(count + 1)]
        if not (offsets[0] >= 14 and offsets[-1] <= node_size - 2 * (count + 1) and
                all(a < b for a, b in zip(offsets, offsets[1:]))):
            raise ValueError('Invalid HFS record offsets')
        for start, end in zip(offsets, offsets[1:]):
            record = node[start:end]
            key_bytes = 2 + struct.unpack_from('>H', record)[0]
            if key_bytes < 2 or key_bytes > len(record) or key_bytes % 2:
                raise ValueError('Invalid HFS key length')
            yield record[:key_bytes], record[key_bytes:]
        if not next_node and number != last:
            raise ValueError('Truncated HFS leaf chain')
        number = next_node


class HFS:
    def __init__(self, image):
        self.image = image
        header = image.read_at(1024, 512)
        if header[:4] not in (b'H+\x00\x04', b'HX\x00\x05'):
            raise ValueError('Expected HFS+ or HFSX volume')
        self.block, self.blocks = struct.unpack_from('>II', header, 40)
        if self.block < 512 or self.block & (self.block - 1) or self.block * self.blocks > image.size:
            raise ValueError('Invalid HFS volume extent')
        self.overflow = {}
        extents = Fork(self, header[192:272], 3, overflow=False)
        for key, data in leaf_records(extents):
            if len(key) != 12 or len(data) != 64:
                raise ValueError('Invalid HFS extent record')
            file_id, start = struct.unpack_from('>II', key, 4)
            token = file_id, key[2], start
            if token in self.overflow:
                raise ValueError('Duplicate HFS extent key')
            self.overflow[token] = [(a, n) for a, n in struct.iter_unpack('>II', data) if n]
        catalog = Fork(self, header[272:352], 4)
        self.folders, self.entries = {}, []
        for key, data in leaf_records(catalog):
            if len(key) < 8 or len(data) < 2:
                raise ValueError('Incomplete HFS catalog record')
            parent, units = struct.unpack_from('>IH', key, 2)
            if 8 + units * 2 != len(key):
                raise ValueError('Invalid HFS catalog name')
            name = key[8:].decode('utf-16-be')
            kind = struct.unpack_from('>h', data)[0]
            if kind in (1, 2):
                file_id = struct.unpack_from('>I', data, 8)[0]
                if kind == 1:
                    if file_id in self.folders:
                        raise ValueError('Duplicate HFS folder ID')
                    self.folders[file_id] = parent, name
                else:
                    if len(data) != 248:
                        raise ValueError('Invalid HFS file record')
                    self.entries.append(dict(parent=parent, name=name, id=file_id, data=data))
        self.files = {}
        for entry in self.entries:
            path = self.folder_path(entry['parent']) + '/' + entry['name']
            if path in self.files:
                raise ValueError('Duplicate HFS path')
            self.files[path] = entry

    def folder_path(self, number):
        names, seen = [], set()
        while number != 2:
            if number in seen or number not in self.folders or len(seen) >= 128:
                raise ValueError('Invalid HFS folder ancestry')
            seen.add(number)
            number, name = self.folders[number]
            names.append(name)
        return '/' + '/'.join(reversed(names)) if names else ''

    def open(self, path):
        entry = self.files[path]
        data = entry['data']
        if data[48:56] == b'hlnkhfs+':
            inode = struct.unpack_from('>I', data, 44)[0]
            matches = [x for x in self.entries if x['name'] == 'iNode' + str(inode)]
            if len(matches) != 1:
                raise ValueError('Missing or ambiguous HFS hard-link backing')
            entry, data = matches[0], matches[0]['data']
        return Fork(self, data[88:168], entry['id'])
