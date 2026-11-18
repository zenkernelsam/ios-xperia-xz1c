"""Host fixtures only: production C codec, split wire records, faults and exact PNG pixels."""
import binascii
from io import BytesIO
from pathlib import Path
import sys
sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'device'))
from scanout_snapshot import ScreenDecoder, RAW_BYTES, CHUNK, fnv64, decode_prle, png_bgr, inspect
from PIL import Image

TOKEN = '0011223344556677'


def line(kind, fields):
    text = ('SWEET SCREEN ' + kind + ' ' + ' '.join(k + '=' + str(v) for k, v in fields.items())).encode()
    return b'\n' + text + f' wire_crc={binascii.crc32(text):08x}\n'.encode()


def records(raw, encoded):
    begin = dict(token=TOKEN, id=1, width=720, height=1280, stride=2160, format='BGR888', bytes=RAW_BYTES,
        codec='PRLE1', encoded=len(encoded), raw_fnv=fnv64(raw), enc_crc=f'{binascii.crc32(encoded):08x}',
        vsync=101, timestamp=1234567, copy_ticks=5678, power_off=0, source_checked=1, source_match=1)
    scan = dict(token=TOKEN, id=1, pa='0x9d400000', map_pa='0x9d400000', map_bytes=RAW_BYTES,
        size='0x50002d0', img='0x50002d0', xy=0, out='0x50002d0', outxy=0, fmt='0x2243f', unpack='0x20001',
        op='0x80000000', qop=0, ctl='0x20020', start=0, before_after=1)
    result = [line('QUEUED', dict(token=TOKEN, id=1, read_only=1)), line('BEGIN', begin), line('SCAN', scan)]
    for at in range(0, len(encoded), CHUNK):
        block = encoded[at:at + CHUNK]
        result.append(line('DATA', dict(token=TOKEN, id=1, offset=at, n=len(block), crc=f'{binascii.crc32(block):08x}', hex=block.hex())))
    result.append(line('END', dict(token=TOKEN, id=1, encoded=len(encoded), blocks=(len(encoded) + CHUNK - 1) // CHUNK,
        raw_fnv=begin['raw_fnv'], enc_crc=begin['enc_crc'], complete=1)))
    return result, begin, scan


def decode(chunks):
    decoder = ScreenDecoder(TOKEN)
    for chunk in chunks: decoder.feed(chunk)
    return decoder.finish()


def main():
    root = Path(sys.argv[1])
    raw = (root / 'full.raw').read_bytes()
    encoded = (root / 'full.prle').read_bytes()
    expected_fnv, expected_crc, expected_size = (root / 'full.checks').read_text().split()
    assert fnv64(raw) == expected_fnv and binascii.crc32(encoded) == int(expected_crc, 16) and len(encoded) == int(expected_size)
    assert decode_prle(encoded, len(raw)) == raw
    blocks, _, _ = records(raw, encoded)
    assert decode(blocks)[0]['pixels'] == raw
    png = png_bgr(raw)
    image = Image.open(BytesIO(png))
    assert image.size == (720, 1280) and image.mode == 'RGB'
    rgb = image.tobytes()
    assert rgb[0::3] == raw[2::3] and rgb[1::3] == raw[1::3] and rgb[2::3] == raw[0::3]
    flat = bytes((5, 127, 249)) * (RAW_BYTES // 3)
    compressed = bytes((255, 5, 127, 249)) * (RAW_BYTES // (128 * 3))
    packets, begin, scan = records(flat, compressed)
    stream = b'ordinary log before\n' + b'kernel log\n'.join(packets) + b'ordinary log after\n'
    for size in (1, 7, 63, 511, 4096, 65536):
        assert decode(stream[i:i + size] for i in range(0, len(stream), size))[0]['pixels'] == flat
    live = ('SWEET LOOP LIVE proto=1 token=' + TOKEN + '\n').encode()
    ack = ('SWEET LOOP ACK FASTBOOT token=' + TOKEN + '\n').encode()
    assert inspect(b'replay prefix\n' + live + stream + ack, TOKEN)[0]['pixels'] == flat

    rejected = 0

    def refuse(chunks):
        nonlocal rejected
        try: decode(chunks)
        except ValueError: rejected += 1; return
        raise AssertionError('Accepted a damaged host fixture')

    refuse(packets[:-1])
    refuse(packets[:4] + packets[5:])
    refuse(packets[:4] + [packets[3]] + packets[4:])
    refuse(packets[:3] + [packets[4], packets[3]] + packets[5:])
    refuse([packets[0], packets[0]] + packets[1:])
    refuse([packets[0]] + packets[2:])
    refuse([packets[0], packets[1]] + packets[3:])
    refuse(packets[1:])
    refuse([packets[0], line('BEGIN', dict(begin, width=719))] + packets[2:])
    refuse([packets[0], line('BEGIN', dict(begin, source_match=0))] + packets[2:])
    refuse([packets[0], line('BEGIN', dict(begin, encoded=RAW_BYTES * 10))] + packets[2:])
    refuse([packets[0], packets[1], line('SCAN', dict(scan, pa='0x9d400003'))] + packets[3:])
    refuse([packets[0], packets[1], line('SCAN', dict(scan, qop=1))] + packets[3:])
    refuse([packets[0], packets[1], line('SCAN', dict(scan, outxy=1))] + packets[3:])
    refuse([packets[0], packets[1], line('SCAN', dict(scan, fmt='0x236ff'))] + packets[3:])
    refuse([packets[0], line('BEGIN', dict(begin, token='ffeeddccbbaa9988'))] + packets[2:])
    refuse([packets[0], line('ABORT', dict(token=TOKEN, id=1, reason='source_changed'))])
    refuse([line('REFUSE', dict(token=TOKEN, reason='busy'))])
    changed = packets[3].replace(b'hex=ff057ff9', b'hex=ff067ff9', 1)
    assert changed != packets[3]
    refuse(packets[:3] + [changed] + packets[4:])
    # Recompute outer wire CRC but retain the independent payload CRC.
    front = changed[1:changed.rindex(b' wire_crc=')]
    changed = b'\n' + front + f' wire_crc={binascii.crc32(front):08x}\n'.encode()
    refuse(packets[:3] + [changed] + packets[4:])
    refuse([packets[0], line('BEGIN', dict(begin, raw_fnv='0000000000000000'))] + packets[2:])
    refuse(packets[:-1] + [packets[-1][:-2]])
    for bad in (b'\xff', b'\x01\x01\x02\x03', b'\xff\x01\x02\x03'):
        try: decode_prle(bad, 3)
        except ValueError: rejected += 1
        else: raise AssertionError('Accepted malformed codec bounds')
    try: inspect(stream + ack, TOKEN)
    except ValueError: rejected += 1
    else: raise AssertionError('Accepted a screenshot without LIVE')
    print(f'PASS screen wire: production C full panel, exact PNG pixels, 6 USB splits, {rejected} rejected faults')


if __name__ == '__main__': main()
