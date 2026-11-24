"""Verify the legacy brightness interface in the locally supplied 20H392 cache."""

import argparse
import hashlib
import json
import struct
from pathlib import Path

from inspect_backlight_contract import (
    A2S, A2S_SHA, AndroidSparseImage, CACHE_PATH, CACHE_SHA, Cache, HFS, ROOT, SPARSE_SHA,
    address_argument, branch_target, digest, loaded_reference, next_symbol,
    page_address, verify_cfstring, verify_reference_symbol,
)


FUNCTIONS = {
    "-[CBDisplayModuleiOS initWithBacklight:queue:display:]": (
        0x1af035708, 2072, "6bb70307233cd54356a2b5dc3cafc22a4a45e4cf2cf5b9b2b7395660db99beb0",
        {1340: "_DisplayCreate", 1656: "_DisplayOpen"}),
    "_DisplayCreate": (
        0x1af0d84d8, 56, "171fcc887d8b53caebc325a184eee6609a77e6e7fd151582f527978b7e5b7c06",
        {40: "_DisplayCreateWithID"}),
    "_DisplayCreateWithID": (
        0x1af0d8510, 22204, "36c7c45a6b378ffc873aea2d3aeaf0969048207bed33d88f1155399dacf26959",
        {644: "_IORegistryEntryCreateCFProperty_stub", 692: "_CFDictionaryGetValue_stub",
         736: "_CFDictionaryGetValue_stub", 784: "_CFNumberGetValue_stub",
         800: "_CFDictionaryGetValue_stub", 848: "_CFNumberGetValue_stub",
         864: "_CFDictionaryGetValue_stub", 912: "_CFNumberGetValue_stub",
         948: "_IORegistryEntryCreateCFProperty_stub"}),
    "_DisplayOpen": (
        0x1af0e1658, 3704, "c55482ae256a248f4493dea51f991191e839575a1c2c275d3a5fbf89e5e50987",
        {804: "_IOMobileFramebufferOpenByName_stub", 856: "_IOMobileFramebufferGetServiceObject_stub",
         1372: "_IORegistryEntryCreateCFProperty_stub", 1400: "_CFNumberGetTypeID_stub",
         1456: "_CFNumberGetValue_stub", 1556: "_IORegistryEntrySearchCFProperty_stub",
         1604: "__DisplayGetUint32FromCFNumberAndRelease"}),
    "___DisplaySetBrightnessWithFade": (
        0x1af0e4cd0, 576, "2bb44d0966c8d452e277c939eae1e98072112894357e528e05222afd1004de2a",
        {372: "___DisplaySetLogicalBrightnessLegacy"}),
    "___DisplaySetLogicalBrightnessLegacy": (
        0x1af0e5708, 1488, "77d7c497eb9c5835fac213b309a31562a08fc47b2efc83c3944d2a0552a694ed",
        {1428: "___DisplaySetPhysicalBrightnessLegacy"}),
    "___DisplaySetPhysicalBrightnessLegacy": (
        0x1af0f15f8, 1252, "dbb09b73cc1baa9f9a796cbdcc2135de8dd0ea6ea58236e94f5191123010d128",
        {1192: "___DisplaySetPhysicalBrightnessLegacyOnThread"}),
    "___DisplaySetPhysicalBrightnessLegacyOnThread": (
        0x1af0ee798, 2436, "78022247aec69825ae4f9b56113ce5409c8c8cc2d8021577b1a7cf7856e7839a",
        {716: "_CFNumberCreate_stub", 804: "_CFNumberCreate_stub", 868: "_CFDictionarySetValue_stub",
         896: "_CFDictionarySetValue_stub", 1024: "_CFNumberCreate_stub", 1084: "_CFDictionarySetValue_stub",
         1108: "_CFDictionaryRemoveValue_stub", 1136: "_IORegistryEntrySetCFProperties_stub"}),
    "__DisplayGetBrightness": (
        0x1af0e95e4, 216, "2559dca98d35ce3a5e8b3a10825608501fc1672d7d06763f6f91151ca957ec5f",
        {132: "_disp_dispatch_sync"}),
    "____DisplayGetBrightness_block_invoke": (
        0x1af0e96bc, 48, "72360877fdac1e4cd391f862ad14de3cadf91118c61f1b1fd725a28512b55f18", {}),
}


def check_words(code, expected):
    if any(struct.unpack_from("<I", code, pos)[0] != word for pos, word in expected.items()):
        raise ValueError("Original 20H392 instruction anchor differs")


def check_function(cache, symbols, name, entry):
    address, length, fingerprint, calls = entry
    verify_reference_symbol(symbols, address, name)
    code = cache.read_at(address, length)
    if next_symbol(symbols, address) != address + length or hashlib.sha256(code).hexdigest() != fingerprint:
        raise ValueError(f"Original function range differs: {name}")
    for offset, target in calls.items():
        verify_reference_symbol(symbols, branch_target(code, address, offset), target)
    return code


def verify_payload(cache, bodies):
    writer = "___DisplaySetPhysicalBrightnessLegacyOnThread"
    code = bodies[writer]
    address = FUNCTIONS[writer][0]
    check_words(code, {
        708: 0xd2800061, 768: 0x1e610800, 772: 0x1e780009, 800: 0xd2800061,
        888: 0xf9400521, 904: 0x721f0108, 1020: 0xd2800061, 1076: 0xf9400921,
        1104: 0xf9400901, 1124: 0xb9405100,
    })
    key_slots = address_argument(code, address, 48, 9)
    if key_slots != 0x1ddc6a820:
        raise ValueError("Unexpected brightness key table")
    verify_cfstring(cache, loaded_reference(cache, code, address, 856, 9, 1, 2), "brightness")
    for offset, encoding, key in ((8, 2, "FadePeriod"), (16, 0, "backlight-level")):
        pointer, = struct.unpack("<Q", cache.read_at(key_slots + offset, 8))
        if pointer >> 40 != encoding:
            raise ValueError(f"Unexpected encoded pointer for {key}")
        verify_cfstring(cache, pointer & ((1 << 40) - 1), key)
    check_words(code, {764: 0xfd406121})
    scale, = struct.unpack("<d", cache.read_at(page_address(code, address, 760, 9) + 0xc0, 8))
    if scale != 65536.0:
        raise ValueError("Original fade conversion scale differs")
    create = "_DisplayCreateWithID"
    create_code = bodies[create]
    check_words(create_code, {564: 0xb94befe8, 572: 0xb9005128, 776: 0x9110f102,
                              780: 0xd2800061, 840: 0x9110d102, 844: 0xd2800061,
                              904: 0x9110e102, 908: 0xd2800061})
    for offset, key in ((632, "IODisplayParameters"), (684, "brightness"), (728, "value"),
                        (792, "min"), (856, "max"), (936, "new-backlight-architecture")):
        verify_cfstring(cache, address_argument(create_code, FUNCTIONS[create][0], offset, 1), key)
    check_words(code, {600: 0xb9443908, 608: 0xb944354a, 612: 0x6b0a0108,
                       620: 0x1e210800, 628: 0x1e64c000, 652: 0x1e612800})

    address = FUNCTIONS["_DisplayOpen"][0]
    code = bodies["_DisplayOpen"]
    check_words(code, {1356: 0xb9405120, 1452: 0xd2800121, 1528: 0xb9406d20, 1552: 0x52800064})
    verify_cfstring(cache, address_argument(code, address, 1364, 1), "nitsThresholdPLC")
    if cache.string(address_argument(code, address, 1536, 1)) != "IOService":
        raise ValueError("Original brightness limit registry plane differs")
    verify_cfstring(cache, address_argument(code, address, 1544, 2), "limit_max_physical_brightness")

    getter = "__DisplayGetBrightness"
    callback = "____DisplayGetBrightness_block_invoke"
    if address_argument(bodies[getter], FUNCTIONS[getter][0], 104, 10) != FUNCTIONS[callback][0]:
        raise ValueError("Original brightness getter callback differs")
    check_words(bodies[callback], {20: 0xf9401408, 24: 0xbd413900, 36: 0xbd001900})
    return {
        "legacy_set": {
            "api": "IORegistryEntrySetCFProperties",
            "service": "DisplayCreate second argument, stored in display context",
            "mandatory_number_keys": ["brightness", "FadePeriod"],
            "conditional_number_key": "backlight-level",
            "number_type_argument": 3,
            "integer_width_bits": 32,
            "fade_scale": int(scale),
            "hardware_duty_mapping_verified": False,
        },
        "open_properties": {
            "backlight_service_number": "nitsThresholdPLC",
            "framebuffer_service_IOService_search": "limit_max_physical_brightness",
            "search_options": 3,
        },
        "legacy_initial_parameters": {
            "backlight_service_dictionary": "IODisplayParameters",
            "brightness_number_keys": ["value", "min", "max"],
            "number_type_argument": 3,
            "base_brightness_conversion": "ceil(normalized * (max - min)) + min",
            "separately_queried_property": "new-backlight-architecture",
        },
        "legacy_get": {"reads_cached_float": True, "direct_hardware_read": False},
    }


def inspect(sparse, symbol_path):
    if digest(sparse) != SPARSE_SHA or digest(symbol_path) != A2S_SHA:
        raise ValueError("Expected the exact 20H392 gl5 and symbol map")
    image = AndroidSparseImage(sparse)
    try:
        fs = HFS(image)
        base = fs.open(CACHE_PATH)
        if hashlib.sha256(base.read_at(0, base.size)).hexdigest() != CACHE_SHA:
            raise ValueError("Unexpected original dyld cache")
        symbols = A2S(symbol_path)
        try:
            cache = Cache(fs)
            bodies = {name: check_function(cache, symbols, name, entry) for name, entry in FUNCTIONS.items()}
            interface = verify_payload(cache, bodies)
        finally:
            symbols.close()
    finally:
        image.close()
    return {
        "system": "iOS16.7.16/20H392", "sparse_sha256": SPARSE_SHA,
        "symbols_sha256": A2S_SHA, "cache_sha256": CACHE_SHA,
        "functions": {name: {"address": hex(address), "bytes": length, "sha256": fingerprint,
                             "verified_calls": {str(offset): target for offset, target in calls.items()}}
                      for name, (address, length, fingerprint, calls) in FUNCTIONS.items()},
        **interface,
        "service_implemented": False, "hardware_function_verified": False, "device_operations": 0,
    }


if __name__ == "__main__":
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
    print(f"Verified {len(report['functions'])} original brightness functions")
