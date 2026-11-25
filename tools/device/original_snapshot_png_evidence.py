"""Decode one original returned PNG from a single current-boot serial stream."""

import hashlib
import re
import struct
import zlib

BEGIN = re.compile(r"SWEET OPNG BEGIN pid=(\d+) call=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) chunks=48 original_return_bytes=1")
BEGIN2 = re.compile(r"SWEET OPNG BEGIN pid=(\d+) call=(\d+) revision=2 bytes=(\d+) fnv=([0-9a-f]{16}) codec=ZLIB1 original_return_bytes=1")
PACKED = re.compile(r"SWEET OPNG PACKED pid=(\d+) call=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) chunks=48")
DATA = re.compile(r"SWEET OPNG DATA pid=(\d+) call=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)")
END = re.compile(r"SWEET OPNG END pid=(\d+) call=(\d+) bytes=(\d+) fnv=([0-9a-f]{16}) unchanged=1 original_return_bytes=1")


def fnv(data):
    value = 0xcbf29ce484222325
    for byte in data:
        value = ((value ^ byte) * 0x100000001b3) & 0xffffffffffffffff
    return f"{value:016x}"


def validate_png(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Original return bytes are not PNG")
    cursor = 8
    kinds, width, height = [], None, None
    while cursor < len(data):
        if len(data) - cursor < 12:
            raise ValueError("Original PNG chunk is truncated")
        size, kind = struct.unpack_from(">I4s", data, cursor)
        if size > len(data) - cursor - 12:
            raise ValueError("Original PNG chunk exceeds its file")
        payload = data[cursor + 8:cursor + 8 + size]
        crc, = struct.unpack_from(">I", data, cursor + 8 + size)
        if zlib.crc32(kind + payload) & 0xffffffff != crc:
            raise ValueError("Original PNG chunk CRC differs")
        if not kinds and kind != b"IHDR":
            raise ValueError("Original PNG lacks first IHDR")
        if kind == b"IHDR":
            if kinds or size != 13:
                raise ValueError("Original PNG header repeated or malformed")
            width, height, depth, color, compression, filter_, interlace = struct.unpack(">IIBBBBB", payload)
            if not 0 < width <= 4096 or not 0 < height <= 4096 or width * height > 4 << 20 or compression or filter_:
                raise ValueError("Original PNG dimensions/codec exceed bound")
        kinds.append(kind)
        cursor += 12 + size
        if kind == b"IEND":
            if size or cursor != len(data):
                raise ValueError("Original PNG terminal chunk differs")
            break
    if not kinds or kinds[-1] != b"IEND" or b"IDAT" not in kinds:
        raise ValueError("Original PNG has no complete image stream")
    return dict(width=width, height=height, chunks=[k.decode("ascii") for k in kinds])


def inspect(text, token, source="return"):
    live, ack = "SWEET LOOP LIVE proto=1 token=" + token, "SWEET LOOP ACK FASTBOOT token=" + token
    if not re.fullmatch("[0-9a-f]{16}", token) or text.count(live) != 1 or text.count(ack) != 1 or text.index(live) >= text.index(ack):
        raise ValueError("Need one current original PNG interval")
    scope = text[text.index(live):text.index(ack)]
    compressed = bool(BEGIN2.search(scope))
    begins, ends = list((BEGIN2 if compressed else BEGIN).finditer(scope)), list(END.finditer(scope))
    if len(begins) != len(ends) or len(begins) != 1 or begins[0].end() >= ends[0].start():
        raise ValueError("Need one complete original PNG export")
    begin, end = begins[0], ends[0]
    if begin.groups() != end.groups():
        raise ValueError("Original PNG identity/size/FNV differs")
    pid, call, size, expected = begin.groups()
    size = int(size)
    if not 8 <= size <= 65536:
        raise ValueError("Original PNG export exceeds bound")
    packed_records = list(PACKED.finditer(scope))
    transmission_bytes = size
    if compressed:
        if len(packed_records) != 1 or packed_records[0].groups()[:2] != (pid,call):
            raise ValueError("Original compressed PNG metadata is incomplete")
        transmission_bytes = int(packed_records[0][3])
        if not 0 < transmission_bytes <= 66000 or not begin.end() < packed_records[0].start() < end.start():
            raise ValueError("Original compressed PNG exceeds its bound")
    elif packed_records:
        raise ValueError("Compressed metadata on an uncompressed original")
    data = bytearray()
    blocks = list(DATA.finditer(scope))
    for record in blocks:
        if record.groups()[:2] != (pid, call):
            raise ValueError("Foreign original PNG block")
        offset, count = int(record[3]), int(record[4])
        payload = bytes.fromhex(record[5])
        if (not begin.end() < record.start() < end.start() or offset != len(data)
                or not 1 <= count <= 48 or count != len(payload) or len(data) + count > transmission_bytes):
            raise ValueError("Missing, repeated or malformed original PNG block")
        data.extend(payload)
    if len(data) != transmission_bytes:
        raise ValueError("Original PNG export bytes/FNV differ")
    if compressed:
        if fnv(data) != packed_records[0][4]:
            raise ValueError("Original compressed PNG FNV differs")
        decoder=zlib.decompressobj()
        restored=decoder.decompress(data,size+1)
        if len(restored)!=size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise ValueError("Original PNG compressed stream is incomplete")
        data=bytearray(restored)
    if fnv(data) != expected:
        raise ValueError("Original PNG FNV differs")
    if scope.count("SWEET OPNG ") != len(blocks) + (3 if compressed else 2):
        raise ValueError("Unknown or truncated original PNG records")
    metadata = validate_png(data)
    if source == "return":
        original_return = re.compile(r"SWEET POSTDRAW END pid=" + pid + r" tid=\d+ call=" + call
            + r" api=png wall_ns=\d+ cpu_ns=\d+ cpu_valid=[01] outcome=return bytes=" + str(size)
            + r" returned_nil=0 original_behavior=[01]")
        returns = list(original_return.finditer(scope))
        if len(returns) != 1 or returns[0].end() >= begin.start():
            raise ValueError("Export is not bound to the original completed PNG return")
    elif source == "loaded":
        original_load = re.compile(r"SWEET PSG END pid=" + pid + r" call=" + call
            + r" api=image_load result=([0-9a-f]{16}) outcome=return original_behavior=1")
        loads = list(original_load.finditer(scope))
        file_records = list(re.finditer(r"SWEET LPNG SOURCE pid=" + pid + r" call=" + call
            + r" result=0 original_image=([0-9a-f]{16}) file_read_only=1", scope))
        if (len(loads) != 1 or len(file_records) != 1 or int(loads[0][1],16) == 0
                or loads[0][1] != file_records[0][1] or not loads[0].end() < file_records[0].start() < begin.start()):
            raise ValueError("Export is not bound to the original loaded snapshot image")
        headers = list(re.finditer(r"SWEET PSG PATH pid=" + pid + r" call=" + call
            + r" bytes=(\d+) truncated=0 getter_error=0 pixels_read=0", scope))
        if len(headers) != 1:
            raise ValueError("Loaded snapshot path metadata incomplete")
        path = bytearray()
        for record in re.finditer(r"SWEET PSG PATH_DATA pid=" + pid + r" call=" + call
            + r" offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)", scope):
            block = bytes.fromhex(record[3])
            if int(record[1]) != len(path) or int(record[2]) != len(block):
                raise ValueError("Loaded snapshot original path bytes are incomplete")
            path.extend(block)
        if len(path) != int(headers[0][1]):
            raise ValueError("Loaded snapshot path length differs")
        metadata["original_loaded_path"] = path.decode("utf-8")
    else:
        raise ValueError("Unknown original PNG source")
    return bytes(data), dict(token=token, pid=int(pid), call=int(call), bytes=size,
        sha256=hashlib.sha256(data).hexdigest(), fnv64=expected, blocks=len(blocks),
        PNG=metadata, original_return_and_export_verified=source=="return",
        original_loaded_file_verified=source=="loaded",
        compression="ZLIB1" if compressed else "none", transmitted_bytes=transmission_bytes,
        original_pixels_not_repaired=True, optical_panel_not_verified=True)
