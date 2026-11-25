"""Audit original iOS16 poster view and asynchronous cutout contracts read-only."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from a2s_lookup import A2S
from hfs_sparse_reader import AndroidSparseImage, HFS
from inspect_native_layout import Cache, CACHE_PATH, CACHE_SHA, SPARSE_SHA, digest, next_symbol
from inspect_poster_routing import A2S_SHA

ROOT = Path(__file__).resolve().parents[2]
TABLES = {
    "SBSWallpaperService": 0x191569148,
    "SBSWallpaperClient": 0x191569258,
    "PRRenderer": 0x18eefbf80,
    "NUNIAstronomyVistaView": 0x1ee9717e0,
    "UIImage": 0x189dfaf40,
    "UIImageView": 0x189e91918,
}
METHODS = {
    ("SBSWallpaperService", "fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:"):
        (0x1915309b0, 180, "v32@0:8q16@?24", "5b881163b3404f11a519e6ac92eb04467b967f82a1aeba10d937fb748641ef69"),
    ("SBSWallpaperService", "fetchMaximalLockScreenContentCutoutBoundsForOrientation:completionHandler:"):
        (0x191530b10, 180, "v32@0:8q16@?24", "9133f23ce9393b408c28bb7508a65c369829d8777e3adc03b3145cabe3c65f51"),
    ("SBSWallpaperClient", "fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:"):
        (0x1915319dc, 16, "v28@0:8I16@?20", "f2060d5821112fe1c01684c9e1ab79a1cc59903fdbcac1ae13612521df3e5d1c"),
    ("SBSWallpaperClient", "fetchMaximalLockScreenContentCutoutBoundsForOrientation:completionHandler:"):
        (0x1915319ec, 16, "v28@0:8I16@?20", "c022241f8174a50fa49d2d6925fb01e3810cfd714843f5e98ad2ded1b7b21a2b"),
    ("PRRenderer", "backgroundView"):
        (0x18eedca6c, 16, "@16@0:8", "395b99ea6a48f5299e4135e53ede66fd089e893af88658e2ff81feb6380f3c53"),
    ("PRRenderer", "floatingView"):
        (0x18eedcbf4, 64, "@16@0:8", "6f0a69f7054aca81e5052b47ddcd03dc6a4efa080e0949bedfc715ada481ead5"),
    ("NUNIAstronomyVistaView", "initWithFrame:configuration:"):
        (0x1ee946d68, 1300, "@56@0:8{CGRect={CGPoint=dd}{CGSize=dd}}16@48",
         "bb25c97f2ab00a2eaaf24d89c4ed8ba083139b4c4785fae26afbd7abd3d5b873"),
    ("UIImage", "initWithContentsOfFile:"):
        (0x188dbee4c, 8, "@24@0:8@16", "aaf0f16ca39584a65c9f6193cb88cee27708d59953dcead8ab8bf093be235ff7"),
    ("UIImageView", "setImage:"):
        (0x188db0028, 420, "v24@0:8@16", "23b11f8b616de30e98aa641fdc3a8a37d04fd9e9b0a605309f484c814450e7e1"),
}
AEGIR_PATH = "/System/Library/CoreServices/AegirProxyApp.app/PlugIns/AegirPoster.appex/AegirPoster"
AEGIR_SHA = "a4abba1e22350a5c577dc3ce551da90ab42606d872509cd50859ef63751454a7"


def macho_range(data, address, length):
    if data[:4] != b"\xcf\xfa\xed\xfe" or length <= 0:
        raise ValueError("Expected original ARM64 image and bounded range")
    cursor = 32
    for _ in range(struct.unpack_from("<I", data, 16)[0]):
        command, size = struct.unpack_from("<II", data, cursor)
        if size < 8 or cursor + size > len(data):
            raise ValueError("Invalid original Mach-O load command")
        if command == 0x19:
            _, vm, _, offset, span = struct.unpack_from("<16sQQQQ", data, cursor + 8)
            if vm <= address and address + length <= vm + span:
                start = offset + address - vm
                if start + length > len(data):
                    raise ValueError("Original code range exceeds file")
                return data[start:start + length]
        cursor += size
    raise ValueError("Original code range is outside mapped file")


def inspect(sparse, symbols_path):
    sparse, symbols_path = Path(sparse).resolve(), Path(symbols_path).resolve()
    before = sparse.stat()
    if digest(sparse) != SPARSE_SHA or digest(symbols_path) != A2S_SHA:
        raise ValueError("Use the pinned gl5 image and original 20H392 symbols")
    image = AndroidSparseImage(sparse)
    symbols = A2S(symbols_path)
    try:
        fs = HFS(image)
        header = fs.open(CACHE_PATH)
        if hashlib.sha256(header.read_at(0, header.size)).hexdigest() != CACHE_SHA:
            raise ValueError("Original cache identity differs")
        cache = Cache(fs)
        rows = []
        for owner, table in TABLES.items():
            flags, count = struct.unpack("<II", cache.read_at(table, 8))
            if flags != 0xc000000f or not 0 < count <= 512:
                raise ValueError("Original method table layout differs: " + owner)
            wanted = {selector for cls, selector in METHODS if cls == owner}
            seen = set()
            for index in range(count):
                entry = table + 8 + index * 12
                _, type_delta, imp_delta = struct.unpack("<iii", cache.read_at(entry, 12))
                imp = entry + 8 + imp_delta
                info = symbols.lookup(imp)
                if not info or info["symbol_offset"] != "0x0" or not info["symbol"].startswith("-[" + owner + " "):
                    continue
                selector = info["symbol"].split(" ", 1)[1][:-1]
                if selector not in wanted:
                    continue
                address, length, encoding, expected_hash = METHODS[owner, selector]
                code = cache.read_at(imp, length)
                actual_hash = hashlib.sha256(code).hexdigest()
                if (imp != address or next_symbol(symbols, imp) != imp + length or selector in seen
                        or cache.string(entry + 4 + type_delta) != encoding
                        or expected_hash is not None and actual_hash != expected_hash):
                    raise ValueError("Original method owner, bytes or ABI differs: " + selector)
                seen.add(selector)
                rows.append(dict(owner=owner, selector=selector, type_encoding=encoding,
                                 implementation=hex(imp), method_list=hex(table),
                                 code_bytes=length, code_sha256=actual_hash, exact_original_owner=True))
            if seen != wanted:
                raise ValueError("Original method inventory incomplete: " + owner)
        fork = fs.open(AEGIR_PATH)
        aegir = fork.read_at(0, fork.size)
        if hashlib.sha256(aegir).hexdigest() != AEGIR_SHA:
            raise ValueError("Original Aegir image identity differs")
        # Actual block invoke stores a CGRect unchanged and resumes its continuation.
        callback = macho_range(aegir, 0x10000ef58, 24)
        if struct.unpack_from("<II", callback, 12) != (0x6d000500, 0x6d010d02):
            raise ValueError("Original callback no longer forwards its four CGRect doubles")
        requests = [
            dict(call_address="0x10000eef0", selector="fetchLockScreenContentCutoutBoundsForOrientation:completionHandler:"),
            dict(call_address="0x10000f010", selector="fetchMaximalLockScreenContentCutoutBoundsForOrientation:completionHandler:"),
        ]
        for call in requests:
            code = macho_range(aegir, int(call["call_address"], 16) - 16, 20)
            if struct.unpack_from("<I", code, 16)[0] >> 26 != 37:
                raise ValueError("Original asynchronous request call is absent")
            call["original_code_sha256"] = hashlib.sha256(code).hexdigest()
        current = sparse.stat()
        if (current.st_size, current.st_mtime_ns, current.st_ino) != (before.st_size, before.st_mtime_ns, before.st_ino):
            raise ValueError("Original image changed during inspection")
        return dict(
            revision=2, system="iOS16.7.16/20H392", sparse_sha256=SPARSE_SHA,
            symbols_sha256=A2S_SHA, cache_sha256=CACHE_SHA, Aegir_sha256=AEGIR_SHA,
            methods=rows, original_requests=requests,
            callback=dict(implementation="0x10000ef58", argument="CGRect / four unchanged doubles",
                          code_sha256=hashlib.sha256(callback).hexdigest()),
            service_orientation_bits=64, client_orientation_bits=32,
            view_getters_observation_adds_no_getter_calls=True,
            asynchronous_reply_and_view_creation_still_require_actual_device_evidence=True,
            source_sha256=digest(Path(__file__)), device_operations=0,
        )
    finally:
        symbols.close()
        image.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sparse", required=True)
    parser.add_argument("--symbols", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output under build/")
    report = inspect(args.sparse, args.symbols)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(json.dumps(dict(methods=len(report["methods"]), callback=report["callback"], device_operations=0)))
