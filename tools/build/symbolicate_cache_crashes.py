"""Resolve current-token cache crash samples against an immutable A2S v2 map."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from a2s_lookup import A2S


def sha_file(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda:stream.read(8*1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def resolve(raw, token, symbols):
    text = raw.decode('ascii', errors='replace')
    live, ack = 'SWEET LOOP LIVE proto=1 token='+token, 'SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live) == text.count(ack) == 1
    start, end = text.index(live), text.index(ack)
    assert start < end
    scope = text[start:end]
    pattern = re.compile(r'CRASH AegirPoster\[(\d+)\] sig (\d+) code (0x[0-9a-f]+) sub (0x[0-9a-f]+) slide (0x[0-9a-f]+) mh (0x[0-9a-f]+)\r?\n'
                         r'CRASH AegirPoster pc (0x[0-9a-f]+) lr (0x[0-9a-f]+) sp (0x[0-9a-f]+) fp (0x[0-9a-f]+) far (0x[0-9a-f]+)\r?\n'
                         r'CRASH AegirPoster bt ([^\r\n]+)\r?\nCRASH AegirPoster reason ([^\r\n]+)')
    crashes = []
    for match in pattern.finditer(scope):
        pid, signal = int(match[1]), int(match[2])
        slide, pc = int(match[5], 16), int(match[7], 16)
        frames = []
        for index, address in enumerate([pc]+[int(value, 16) for value in match[12].split()]):
            frame = dict(index=index, actual_pc=hex(address), instruction_sample=index == 0)
            if address >= 0x180000000+slide:
                unslid = address-slide
                value = symbols.lookup(unslid)
                if value:
                    frame.update(value, unslid_pc=hex(unslid), current_boot_slide_used=True,
                                 nearest_symbol_is_not_exact_function_boundary_proof=True)
            else:
                frame['not_resolved_against_shared_cache'] = True
            frames.append(frame)
        crashes.append(dict(pid=pid, signal=signal, code=match[3], subcode=match[4],
                            actual_shared_cache_slide=match[5], actual_main_header=match[6],
                            reason=match[13], frames=frames, current_boot_slide_observed=True))
    assert crashes and len(crashes) == len(re.findall(r'CRASH AegirPoster\[\d+\] sig ', scope))
    return crashes


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--raw', required=True)
    parser.add_argument('--raw-sha256', required=True)
    parser.add_argument('--token', required=True)
    parser.add_argument('--symbols', required=True)
    parser.add_argument('--symbols-sha256', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    assert re.fullmatch('[0-9a-f]{16}', args.token)
    assert all(re.fullmatch('[0-9a-f]{64}', value) for value in (args.raw_sha256, args.symbols_sha256))
    raw = Path(args.raw).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == args.raw_sha256
    assert sha_file(args.symbols) == args.symbols_sha256
    symbols = A2S(args.symbols)
    try:
        crashes = resolve(raw, args.token, symbols)
        report = dict(token=args.token, raw_sha256=args.raw_sha256, crashes=crashes,
                      symbol_map_sha256=args.symbols_sha256, symbol_map_bytes=Path(args.symbols).stat().st_size,
                      symbol_map_format='A2S v2; 12-byte header and 12-byte sorted address/offset record',
                      symbol_map_count=symbols.count, symbol_reader_source_sha256=sha_file(Path(__file__).with_name('a2s_lookup.py')),
                      diagnostic_tool_source_sha256=sha_file(__file__), post_ACK_exit_period_excluded=True,
                      sample_locations_do_not_prove_root_cause=True, no_kernel_or_allocator_changes=True)
    finally:
        symbols.close()
    output = Path(args.output)
    assert not output.exists()
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(dict(token=args.token, crash_count=len(crashes), source_only_diagnostic=True)))
