"""Decode source-bound lossless Sony scanout records; this module never opens COM."""
import argparse
import binascii
import hashlib
import json
from pathlib import Path
import re
import struct
import zlib

WIDTH, HEIGHT, STRIDE = 720, 1280, 2160
RAW_BYTES = STRIDE * HEIGHT
MAX_ENCODED = RAW_BYTES + (WIDTH * HEIGHT + 127) // 128
CHUNK, FRAME_LIMIT = 128, 4
PREFIX = b'SWEET SCREEN '


def fnv64(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return f'{value:016x}'


def decode_prle(data, expected_bytes):
    if not 0 < expected_bytes <= RAW_BYTES or expected_bytes % 3 or not 0 < len(data) <= MAX_ENCODED:
        raise ValueError('Invalid scanout codec bounds')
    output = bytearray()
    at = 0
    while at < len(data):
        tag = data[at]
        at += 1
        count = (tag & 127) + 1
        size = 3 if tag & 128 else count * 3
        if size > len(data) - at or count * 3 > expected_bytes - len(output):
            raise ValueError('Truncated or overflowing scanout codec packet')
        block = data[at:at + size]
        output.extend(block * count if tag & 128 else block)
        at += size
    if len(output) != expected_bytes:
        raise ValueError('Incomplete scanout pixels')
    return bytes(output)


def png_bgr(raw):
    if len(raw) != RAW_BYTES:
        raise ValueError('Unknown scanout dimensions')
    rows = bytearray((STRIDE + 1) * HEIGHT)
    for y in range(HEIGHT):
        bgr = raw[y * STRIDE:(y + 1) * STRIDE]
        rgb = bytearray(STRIDE)
        rgb[0::3], rgb[1::3], rgb[2::3] = bgr[2::3], bgr[1::3], bgr[0::3]
        start = y * (STRIDE + 1)
        rows[start + 1:start + 1 + STRIDE] = rgb

    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', binascii.crc32(kind + data))

    return (b'\x89PNG\r\n\x1a\n' +
        chunk(b'IHDR', struct.pack('>2I5B', WIDTH, HEIGHT, 8, 2, 0, 0, 0)) +
        chunk(b'sRGB', b'\0') + chunk(b'IDAT', zlib.compress(rows, 6)) + chunk(b'IEND', b''))


def number(value):
    if not re.fullmatch(r'(?:0x[0-9a-f]+|[0-9]+)', value):
        raise ValueError('Invalid scanout integer')
    return int(value, 16 if value.startswith('0x') else 10)


def hex_value(value, count):
    if not re.fullmatch('[0-9a-f]{' + str(count) + '}', value):
        raise ValueError('Invalid scanout hash')
    return int(value, 16)


class ScreenDecoder:
    """Every record/chunk/frame is checked; damaged transfers are never repaired."""
    def __init__(self, token):
        if not re.fullmatch('[0-9a-f]{16}', token):
            raise ValueError('Invalid current boot token')
        self.token = token
        self.pending = bytearray()
        self.queued = None
        self.current = None
        self.frames = []
        self.last_id = 0

    def feed(self, data):
        self.pending.extend(data)
        while True:
            end = self.pending.find(b'\n')
            if end < 0:
                if len(self.pending) > 1024 * 1024:
                    raise ValueError('Unbounded serial line')
                return
            line = bytes(self.pending[:end]).rstrip(b'\r')
            del self.pending[:end + 1]
            if line.startswith(PREFIX):
                self.record(line)

    def record(self, line):
        if line.startswith((b'SWEET SCREEN READY ', b'SWEET SCREEN UNAVAILABLE ')):
            return  # Status is not an image and cannot satisfy any frame requirement.
        if len(line) > 511:
            raise ValueError('Unbounded scanout record')
        wire = re.fullmatch(rb'(.+) wire_crc=([0-9a-f]{8})', line)
        if not wire or binascii.crc32(wire[1]) != int(wire[2], 16):
            raise ValueError('Scanout record CRC mismatch')
        words = wire[1].decode('ascii').split()
        kind = words[2]
        values = {}
        for word in words[3:]:
            if word.count('=') != 1:
                raise ValueError('Malformed scanout field')
            key, value = word.split('=')
            if key in values:
                raise ValueError('Duplicate scanout field')
            values[key] = value
        if values.get('token') != self.token:
            raise ValueError('Scanout boot token differs')
        if kind in ('ABORT', 'REFUSE'):
            raise ValueError('Device refused scanout: ' + values.get('reason', 'unknown'))
        ident = number(values.get('id', 'invalid'))
        if not 1 <= ident <= FRAME_LIMIT:
            raise ValueError('Invalid scanout frame id')

        def keys(expected):
            if set(values) != {'token', 'id'} | set(expected.split()):
                raise ValueError('Scanout metadata fields differ: ' + kind)

        if kind == 'QUEUED':
            keys('read_only')
            if values['read_only'] != '1' or self.queued is not None or self.current is not None or ident != self.last_id + 1:
                raise ValueError('Unexpected scanout request order')
            self.queued = ident
        elif kind == 'BEGIN':
            keys('width height stride format bytes codec encoded raw_fnv enc_crc vsync timestamp copy_ticks power_off source_checked source_match')
            if self.queued != ident or self.current is not None:
                raise ValueError('Scanout begins without its request')
            for key, expected in {'width': WIDTH, 'height': HEIGHT, 'stride': STRIDE, 'bytes': RAW_BYTES,
                                  'source_checked': 1, 'source_match': 1}.items():
                if number(values[key]) != expected:
                    raise ValueError('Unknown scanout contract: ' + key)
            if values['format'] != 'BGR888' or values['codec'] != 'PRLE1':
                raise ValueError('Unsupported scanout format/codec')
            if not 0 < number(values['encoded']) <= MAX_ENCODED or number(values['power_off']) not in (0, 1):
                raise ValueError('Invalid scanout bounds/power metadata')
            if number(values['vsync']) <= 0 or number(values['timestamp']) <= 0 or number(values['copy_ticks']) < 0:
                raise ValueError('Invalid scanout capture time')
            hex_value(values['raw_fnv'], 16)
            hex_value(values['enc_crc'], 8)
            self.current = {'id': ident, 'begin': values, 'scan': None, 'encoded': bytearray(), 'blocks': 0}
        elif kind == 'SCAN':
            keys('pa map_pa map_bytes size img xy out outxy fmt unpack op qop ctl start before_after')
            frame = self.current
            if frame is None or frame['id'] != ident or frame['scan'] is not None or frame['blocks']:
                raise ValueError('Unexpected scanout register record')
            physical = number(values['pa'])
            if not 0 < physical < physical + RAW_BYTES <= 1 << 32 or number(values['map_pa']) != physical or number(values['map_bytes']) != RAW_BYTES:
                raise ValueError('Scanout physical mapping differs')
            size = HEIGHT << 16 | WIDTH
            for key, expected in {'size': size, 'img': size, 'out': size, 'xy': 0, 'outxy': 0,
                                  'fmt': 0x2243f, 'unpack': 0x20001, 'op': 0x80000000, 'qop': 0,
                                  'ctl': 0x20020, 'before_after': 1}.items():
                if number(values[key]) != expected:
                    raise ValueError('Unsupported actual scanout register: ' + key)
            if number(values['start']) not in (0, 1):
                raise ValueError('Unknown scanout start state')
            frame['scan'] = values
        elif kind == 'DATA':
            keys('offset n crc hex')
            frame = self.current
            if frame is None or frame['id'] != ident or frame['scan'] is None:
                raise ValueError('Scanout pixels lack checked source')
            offset, count = number(values['offset']), number(values['n'])
            total = number(frame['begin']['encoded'])
            if offset != len(frame['encoded']) or not 1 <= count <= CHUNK or count > total - offset or (count < CHUNK and offset + count != total):
                raise ValueError('Missing, duplicated or reordered scanout chunk')
            if not re.fullmatch('[0-9a-f]{' + str(count * 2) + '}', values['hex']):
                raise ValueError('Truncated scanout pixels')
            payload = bytes.fromhex(values['hex'])
            if binascii.crc32(payload) != hex_value(values['crc'], 8):
                raise ValueError('Scanout chunk CRC mismatch')
            frame['encoded'].extend(payload)
            frame['blocks'] += 1
        elif kind == 'END':
            keys('encoded blocks raw_fnv enc_crc complete')
            frame = self.current
            if frame is None or frame['id'] != ident or frame['scan'] is None:
                raise ValueError('Unexpected scanout end')
            begin = frame['begin']
            if values['complete'] != '1' or number(values['blocks']) != frame['blocks'] or number(values['encoded']) != len(frame['encoded']) or len(frame['encoded']) != number(begin['encoded']):
                raise ValueError('Incomplete scanout transfer')
            if values['raw_fnv'] != begin['raw_fnv'] or values['enc_crc'] != begin['enc_crc'] or binascii.crc32(frame['encoded']) != hex_value(begin['enc_crc'], 8):
                raise ValueError('Scanout frame CRC mismatch')
            raw = decode_prle(frame['encoded'], RAW_BYTES)
            if fnv64(raw) != begin['raw_fnv']:
                raise ValueError('Scanout original pixel hash differs')
            self.frames.append({'id': ident, 'begin': begin, 'scan': frame['scan'], 'blocks': frame['blocks'], 'pixels': raw})
            self.current = self.queued = None
            self.last_id = ident
        else:
            raise ValueError('Unknown scanout record')

    def finish(self):
        if self.current is not None or self.queued is not None or not self.frames:
            raise ValueError('No complete checked scanout frame')
        if self.pending.startswith(PREFIX):
            raise ValueError('Truncated scanout record')
        return self.frames


def inspect(raw, token):
    live = ('SWEET LOOP LIVE proto=1 token=' + token).encode()
    ack = ('SWEET LOOP ACK FASTBOOT token=' + token).encode()
    if raw.count(live) != 1:
        raise ValueError('Screenshot requires one validated current-boot LIVE')
    start = raw.index(b'\n', raw.index(live)) + 1
    end = raw.find(ack, start)
    decoder = ScreenDecoder(token)
    decoder.feed(raw[start:end if end >= 0 else len(raw)])
    return decoder.finish()


def binding(record):
    from diagnostic_candidate import verify_record
    config = verify_record(record)
    if record['version'] < 730 or 'sfscreen=1' not in record['cmdline'].split():
        raise ValueError('Kernel was not packaged for the checked screenshot reader')
    if 'sweetfb_noconsole=1' not in record['cmdline'].split():
        raise ValueError('Screenshot requires the separate serial log without concurrent panel console writes')
    sources = config['source_hashes']
    return dict(protocol=1, width=WIDTH, height=HEIGHT, stride=STRIDE, format='BGR888', codec='PRLE1',
        reader_source_sha256=sources['iokit/Kernel/arm/SweetFramebuffer.cpp'],
        codec_source_sha256=sources['iokit/Kernel/arm/SweetFramebufferScreen.h'],
        runtime_source_sha256=sources['iokit/Kernel/arm/SweetFramebufferScreenRuntime.h'])


def write_frame(directory, frame, record):
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    stem = f'scanout-{frame["id"]:08d}'
    raw, png = frame['pixels'], png_bgr(frame['pixels'])
    metadata = dict(id=frame['id'], token=record['token'], kernel_sha256=record['kernel_sha256'],
        image_sha256=record['sha256'], binding=record['screen_snapshot_binding'], begin=frame['begin'],
        scan=frame['scan'], blocks=frame['blocks'], raw_file=stem + '.bgr', png_file=stem + '.png',
        raw_sha256=hashlib.sha256(raw).hexdigest(), png_sha256=hashlib.sha256(png).hexdigest(),
        pixel_values_preserved=True, verified=True, optical_panel_verified=False)
    for name, data in ((stem + '.bgr', raw), (stem + '.png', png),
                       (stem + '.json', (json.dumps(metadata, indent=2) + '\n').encode())):
        with (directory / name).open('xb') as file:
            file.write(data)
    return metadata


def verify_saved(directory, record, reported, raw):
    if record['screen_snapshot_binding'] != binding(record) or reported.get('verified') is not True or reported.get('optical_panel_verified') is not False:
        raise ValueError('Scanout source/evidence binding differs')
    frames = inspect(raw, record['token'])
    if len(frames) != len(reported.get('frames', [])) or len(frames) != 1:
        raise ValueError('Screenshot must contain exactly one requested complete frame')
    for frame, metadata in zip(frames, reported['frames']):
        stem = f'scanout-{frame["id"]:08d}'
        if metadata['raw_file'] != stem + '.bgr' or metadata['png_file'] != stem + '.png':
            raise ValueError('Unknown screenshot file names')
        saved = json.loads((Path(directory) / (stem + '.json')).read_bytes())
        if saved != metadata or saved['token'] != record['token'] or saved['kernel_sha256'] != record['kernel_sha256'] or saved['image_sha256'] != record['sha256'] or saved['binding'] != binding(record):
            raise ValueError('Saved scanout metadata differs')
        pixels = (Path(directory) / metadata['raw_file']).read_bytes()
        png = (Path(directory) / metadata['png_file']).read_bytes()
        if pixels != frame['pixels'] or png != png_bgr(pixels) or hashlib.sha256(pixels).hexdigest() != metadata['raw_sha256'] or hashlib.sha256(png).hexdigest() != metadata['png_sha256']:
            raise ValueError('Saved screenshot pixels differ from the original serial bytes')
        if metadata['begin'] != frame['begin'] or metadata['scan'] != frame['scan'] or metadata['blocks'] != frame['blocks']:
            raise ValueError('Saved scanout source registers differ')
    return dict(protocol=1, frames=reported['frames'], verified=True, optical_panel_verified=False,
        current_boot_source_and_payload_checked=True, format='BGR888', width=WIDTH, height=HEIGHT)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raw', required=True)
    parser.add_argument('--artifact', required=True, help='The exact trial artifacts.json')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    record = json.loads(Path(args.artifact).read_bytes())['artifacts'][0]
    assert record['screen_snapshot_binding'] == binding(record)
    output = Path(args.output)
    assert not output.exists(), 'Preserve previous screenshot outputs'
    frames = inspect(Path(args.raw).read_bytes(), record['token'])
    report = [write_frame(output, frame, record) for frame in frames]
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
