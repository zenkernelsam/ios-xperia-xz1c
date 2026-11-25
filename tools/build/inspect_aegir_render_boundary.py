"""Verify the original Aegir draw and binding call sites from immutable gl5."""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from a2s_lookup import A2S
from hfs_sparse_reader import AndroidSparseImage, HFS
from inspect_native_layout import Cache, SPARSE_SHA, digest
from inspect_poster_routing import A2S_SHA

ROOT = Path(__file__).resolve().parents[2]
RANGES = (
    ("background", 0x1ee956b40, 540, "5f2ad1025cb6fa4bc66b8a8846056d204b9e3c7643d77b26ed77738989b4ac22"),
    ("scene", 0x1ee9579e4, 2008, "c57cbc4d9806acaeaafc05bb554e07e35a365cc544ea6124c83686efb4b5301c"),
    ("bloom", 0x1ee9581d4, 712, "41daa3c03c0ae047ff1079dd35cccf3104299a3c45d261146799f9d75b0cea8a"),
    ("post", 0x1ee95849c, 804, "6aa56f69652eabc5ae02c9792b93a1629b8fbcba1d19537d9da884b3de575fff"),
)
CALLS = (
    (0x1ee956bf8, "setVertexBuffer:offset:atIndex:", "uniform vertex slot 1"),
    (0x1ee956c24, "setVertexBuffer:offset:atIndex:", "quad vertex slot 0"),
    (0x1ee956c68, "setFragmentBuffer:offset:atIndex:", "uniform fragment slot 0"),
    (0x1ee956cb0, "drawPrimitives:vertexStart:vertexCount:", "triangle strip / four vertices"),
    (0x1ee956cdc, "setVertexBuffer:offset:atIndex:", "star vertex slot 0"),
    (0x1ee956d20, "setFragmentBuffer:offset:atIndex:", "same fragment slot 0 remains bound"),
    (0x1ee956d34, "drawPrimitives:vertexStart:vertexCount:", "triangles / 4212 vertices"),
)


def inspect(sparse, symbols):
    sparse, symbols = Path(sparse).resolve(), Path(symbols).resolve()
    before = sparse.stat()
    if digest(sparse) != SPARSE_SHA or digest(symbols) != A2S_SHA:
        raise ValueError("Use the pinned original 20H392 gl5 and symbols")
    image = AndroidSparseImage(sparse)
    mapping = A2S(symbols)
    try:
        cache = Cache(HFS(image))
        ranges, calls = [], []
        for name, address, size, expected in RANGES:
            data = cache.read_at(address, size)
            if hashlib.sha256(data).hexdigest() != expected:
                raise ValueError("Original render range differs: " + name)
            ranges.append(dict(name=name, address=hex(address), bytes=size, sha256=expected))
        for address, selector, purpose in CALLS:
            word, = struct.unpack("<I", cache.read_at(address, 4))
            if word >> 26 != 0b100101:
                raise ValueError("Original direct call is absent")
            displacement = word & 0x3ffffff
            if displacement & 0x2000000:
                displacement -= 0x4000000
            target = address + displacement * 4
            symbol = mapping.lookup(target)
            if not symbol or symbol["symbol_offset"] != "0x0" or symbol["symbol"] != "_objc_msgSend$" + selector:
                raise ValueError("Original selector call target differs")
            calls.append(dict(address=hex(address), target=hex(target), selector=selector, purpose=purpose,
                              instruction_sha256=hashlib.sha256(cache.read_at(address, 4)).hexdigest()))
        after = sparse.stat()
        if (before.st_size, before.st_mtime_ns, before.st_ino) != (after.st_size, after.st_mtime_ns, after.st_ino):
            raise ValueError("Original sparse input changed")
        return dict(revision=1, sparse_sha256=SPARSE_SHA, symbols_sha256=A2S_SHA,
                    original_ranges=ranges, original_call_sites=calls,
                    original_encoder_keeps_fragment_slot0_for_second_draw=True,
                    actual_shader_reflection_and_command_error_need_device_evidence=True,
                    original_bytes_not_published=True, device_operations=0,
                    tool_sha256=digest(Path(__file__)))
    finally:
        mapping.close()
        image.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sparse", required=True)
    parser.add_argument("--symbols", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output under formal build/")
    report = inspect(args.sparse, args.symbols)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps(dict(ranges=len(report["original_ranges"]), calls=len(report["original_call_sites"]),
                          device_operations=0)))
