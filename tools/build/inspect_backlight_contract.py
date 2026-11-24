"""Verify the CoreBrightness backlight discovery contract in the local 20H392 image."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_native_layout import (
    A2S, A2S_SHA, AndroidSparseImage, CACHE_PATH, CACHE_SHA, Cache, HFS,
    ROOT, SPARSE_SHA, digest, next_symbol,
)


OWNER = "CBDisplayContaineriOS"
TABLE = 0x1af13ff50
METHODS = {
    "findBacklight": (
        0x1af0cb224, 608, "B16@0:8",
        "2c78922ab7ef2480d4a0f609180d0464706aa5c0c1bb55c6818f4d19efe7e414",
        {136: "_CFDictionaryCreate_stub", 200: "_CFDictionaryCreate_stub",
         252: "_IOServiceGetMatchingServices_stub", 304: "_IOIteratorNext_stub",
         372: "_IOObjectConformsTo_stub", 508: "_objc_msgSend$handleDisplayArrival:"},
        (372,),
    ),
    "initWithBacklightService:": (
        0x1af0c9f20, 1940, "@20@0:8I16",
        "4c4f189cb4634f7255d667e040e9118bca451375601d49b76664c096a88e37c0",
        {432: "_IORegistryEntryGetRegistryEntryID_stub", 1036: "_IOObjectConformsTo_stub",
         1164: "_IORegistryEntrySearchCFProperty_stub", 1188: "_objc_opt_class_stub",
         1200: "_objc_opt_isKindOfClass_stub", 1232: "_objc_msgSend$initWithUUIDString:"},
        (1036,),
    ),
    "handleDisplayArrival:": (
        0x1af0cba00, 1892, "B20@0:8I16",
        "dcb60c08046789488357291cd666fb1e38d7b3f5aa9fb743bf15a2794752f4f7",
        {156: "_IORegistryEntryGetRegistryEntryID_stub", 184: "_IOObjectCopyClass_stub",
         1044: "_IOObjectConformsTo_stub", 1128: "_IOObjectConformsTo_stub",
         1224: "_IORegistryEntrySearchCFProperty_stub", 1248: "_objc_opt_class_stub",
         1260: "_objc_opt_isKindOfClass_stub", 1292: "_objc_msgSend$initWithUUIDString:"},
        (1044, 1128),
    ),
}


def sign_extend(value, width):
    sign = 1 << (width - 1)
    return (value ^ sign) - sign


def branch_target(code, address, offset):
    instruction, = struct.unpack_from("<I", code, offset)
    if instruction >> 26 != 37:
        raise ValueError("Expected an original AArch64 BL")
    return address + offset + 4 * sign_extend(instruction & 0x3ffffff, 26)


def page_address(code, address, offset, register):
    page, = struct.unpack_from("<I", code, offset)
    if (page & 0x9f00001f) != (0x90000000 | register):
        raise ValueError("Expected the original ADRP")
    immediate = ((page >> 5) & 0x7ffff) << 2 | (page >> 29) & 3
    return ((address + offset) & ~0xfff) + (sign_extend(immediate, 21) << 12)


def address_argument(code, address, offset, register):
    base = page_address(code, address, offset, register)
    add, = struct.unpack_from("<I", code, offset + 4)
    if (add & 0xffc003ff) != (0x91000000 | (register << 5) | register):
        raise ValueError("Expected the original ADD for an address argument")
    return base + ((add >> 10) & 0xfff)


def loaded_reference(cache, code, address, offset, base_register, target_register, encoding):
    base = page_address(code, address, offset, base_register)
    load, = struct.unpack_from("<I", code, offset + 4)
    if (load & 0xffc003ff) != (0xf9400000 | (base_register << 5) | target_register):
        raise ValueError("Expected the original LDR for a cache reference")
    slot = base + (((load >> 10) & 0xfff) * 8)
    pointer, = struct.unpack("<Q", cache.read_at(slot, 8))
    if pointer >> 40 != encoding:
        raise ValueError("Unexpected encoding of the original cache reference")
    return pointer & ((1 << 40) - 1)


def verify_cfstring(cache, address, expected):
    _, kind, pointer, length = struct.unpack("<QQQQ", cache.read_at(address, 32))
    if (kind, pointer >> 40, length) != (0x7c8, 4, len(expected)):
        raise ValueError("Unexpected original constant string")
    if cache.read_at(pointer & ((1 << 40) - 1), length + 1) != expected.encode("ascii") + b"\0":
        raise ValueError("Unexpected original constant string text")


def verify_reference_symbol(symbols, address, expected):
    target = symbols.lookup(address)
    if not target or target["symbol_offset"] != "0x0" or target["symbol"] != expected:
        raise ValueError(f"Unexpected original reference, expected {expected}")


def verify_matching_dictionary(cache, symbols, code, address):
    verify_cfstring(cache, loaded_reference(cache, code, address, 44, 8, 8, 2), "backlight-control")
    verify_cfstring(cache, loaded_reference(cache, code, address, 160, 9, 9, 8), "IOPropertyMatch")
    verify_reference_symbol(symbols, loaded_reference(cache, code, address, 68, 8, 8, 2), "_kCFBooleanTrue")
    return {"IOPropertyMatch": {"backlight-control": True}}


def verify_property_search(cache, symbols, code, address, call_offset):
    plane = cache.string(address_argument(code, address, call_offset - 20, 1))
    key_address = address_argument(code, address, call_offset - 12, 2)
    options, = struct.unpack_from("<I", code, call_offset - 4)
    if (plane, options) != ("IOService", 0x52800064):
        raise ValueError("Unexpected original registry search arguments")
    verify_cfstring(cache, key_address, "kUSBContainerID")
    class_address = loaded_reference(cache, code, address, call_offset + 16, 8, 0, 2)
    verify_reference_symbol(symbols, class_address, "_OBJC_CLASS_$_NSString")
    return {"plane": plane, "key": "kUSBContainerID", "options": 3,
            "result_class_checked": "NSString", "uuid_initializer_observed": True}


def verify_methods(cache, symbols):
    flags, count = struct.unpack("<II", cache.read_at(TABLE, 8))
    if flags != 0xc000000f or not 0 < count <= 512:
        raise ValueError("Unexpected CoreBrightness method table")
    entries = {}
    for index, (_, type_delta, imp_delta) in enumerate(
        struct.iter_unpack("<iii", cache.read_at(TABLE + 8, count * 12))
    ):
        entry = TABLE + 8 + index * 12
        implementation = entry + 8 + imp_delta
        entries.setdefault(implementation, []).append(entry + 4 + type_delta)

    records = {}
    for selector, (address, length, expected_types, expected_hash, calls, class_checks) in METHODS.items():
        symbol = symbols.lookup(address)
        type_addresses = entries.get(address, [])
        if (not symbol or symbol["symbol_offset"] != "0x0"
                or symbol["symbol"] != f"-[{OWNER} {selector}]"
                or len(type_addresses) != 1 or next_symbol(symbols, address) != address + length):
            raise ValueError(f"Method boundary or ownership differs: {selector}")
        types = cache.string(type_addresses[0])
        code = cache.read_at(address, length)
        if types != expected_types or hashlib.sha256(code).hexdigest() != expected_hash:
            raise ValueError(f"Method bytes or ABI differ: {selector}")

        verified_calls = []
        for offset, expected_name in calls.items():
            target = symbols.lookup(branch_target(code, address, offset))
            if not target or target["symbol_offset"] != "0x0" or target["symbol"] != expected_name:
                raise ValueError(f"Original call differs: {selector}+{offset:#x}")
            record = {"offset": offset, "symbol": expected_name}
            if offset in class_checks:
                actual = cache.string(address_argument(code, address, offset - 8, 1))
                if actual != "AppleARMBacklight":
                    raise ValueError("Unexpected original class argument")
                record["class_argument"] = actual
            if expected_name == "_IORegistryEntrySearchCFProperty_stub":
                record["arguments_and_result_use"] = verify_property_search(cache, symbols, code, address, offset)
            verified_calls.append(record)

        records[selector] = {"implementation": hex(address), "type_encoding": types, "code_bytes": length,
                             "code_sha256": expected_hash, "verified_calls": verified_calls}
        if selector == "findBacklight":
            records[selector]["matching_dictionary"] = verify_matching_dictionary(cache, symbols, code, address)
    return records


def inspect(sparse, symbol_path):
    if digest(sparse) != SPARSE_SHA or digest(symbol_path) != A2S_SHA:
        raise ValueError("Expected pinned 20H392 gl5 and symbol map")
    image = AndroidSparseImage(sparse)
    try:
        fs = HFS(image)
        base = fs.open(CACHE_PATH)
        if hashlib.sha256(base.read_at(0, base.size)).hexdigest() != CACHE_SHA:
            raise ValueError("Unexpected original dyld cache")
        symbols = A2S(symbol_path)
        try:
            methods = verify_methods(Cache(fs), symbols)
        finally:
            symbols.close()
    finally:
        image.close()
    return {"system": "iOS16.7.16/20H392", "sparse_sha256": SPARSE_SHA, "symbols_sha256": A2S_SHA,
            "cache_sha256": CACHE_SHA, "owner": OWNER, "method_list": hex(TABLE), "methods": methods,
            "brightness_write_abi_verified": False, "hardware_function_verified": False, "device_operations": 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sparse", type=Path, required=True)
    parser.add_argument("--symbols", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "build") or output.exists():
        raise ValueError("Use a new output file under build/")
    report = inspect(args.sparse, args.symbols)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2)
        stream.write("\n")
    print(f"Verified {len(report['methods'])} original discovery methods; no device operations")


if __name__ == "__main__":
    main()
