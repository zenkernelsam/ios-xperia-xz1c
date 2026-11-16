"""Bind every primed shader to actual original compile hashes and native proof."""
import argparse
import json
import re
from pathlib import Path
from failed_trial_evidence import verify as verify_failed
from trial_evidence import verify as verify_trial
from native_prerequisite import verify as verify_native


def scope(directory, token):
    text = (Path(directory)/'cycle-01/serial.raw.log').read_text(encoding='ascii', errors='replace')
    text = text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    return re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', text)


def compile_pairs(text):
    pattern = r'SWEET AIR RENDER compile_begin stage=vertex bytes=(\d+) hash=([0-9a-f]{16})(.*?)SWEET AIR RENDER compile_begin stage=fragment bytes=(\d+) hash=([0-9a-f]{16})'
    return {(int(vs), vh, int(fs), fh) for vs, vh, middle, fs, fh in re.findall(pattern, text, re.S)
            if 'SWEET AIR RENDER compile_begin stage=vertex' not in middle}


def verify(original_directory, prime_directory):
    original = verify_failed(original_directory)
    prime = verify_trial(prime_directory, 'gpu', original['kernel_sha256'], original['version'])
    native = verify_native(prime_directory)
    old_row = json.loads((Path(original_directory)/'results.json').read_bytes())[-1]['artifact']
    new_row = json.loads((Path(prime_directory)/'results.json').read_bytes())[-1]['artifact']
    assert new_row['exception_observer'].get('program_cache_prime')
    assert new_row['metal_resources'] == old_row['metal_resources']
    assert new_row['exception_observer']['program_cache_scope'] == old_row['exception_observer']['program_cache_scope']
    before, after = scope(original_directory, original['token']), scope(prime_directory, prime['token'])
    requests = {(int(v), int(f)) for v, f in re.findall(r'SWEET PROGRAM CACHE request result=0 vertex_bytes=(\d+) fragment_bytes=(\d+) immutable=1', before)}
    actual_compiles = compile_pairs(before)
    begins = list(re.finditer(r'SWEET PROGRAM CACHE prime_begin vertex_bytes=(\d+) fragment_bytes=(\d+) source_verified=1', after))
    assert begins
    samples = []
    for n, begin in enumerate(begins):
        request = tuple(map(int, begin.groups()))
        assert request in requests
        limit = begins[n+1].start() if n+1 < len(begins) else len(after)
        block = after[begin.end():limit]
        entries = list(re.finditer(r'SWEET PROGRAM CACHE prime_entry result=0 cache_state=2 reason=', block))
        assert len(entries) == 1
        block = block[:entries[0].end()]
        compiled = compile_pairs(block)
        assert len(compiled) == 1 and compiled <= actual_compiles, 'Primed wrapper differs from actual runtime source'
        links = re.findall(r'SWEET AIR RENDER compile_end stage=link wall_us=(\d+) cpu_us=(\d+) cpu_clock=1 status=1', block)
        stored = re.findall(r'SWEET PROGRAM CACHE stored result=0 bytes=(\d+) format=(0x[0-9a-f]+) error=0 exact_build=1', block)
        assert len(links) == len(stored) == 1 and 0 < int(stored[0][0]) <= 8 << 20
        samples.append(dict(request_vertex_bytes=request[0], request_fragment_bytes=request[1],
                            actual_wrapper=next(iter(compiled)), link_wall_us=int(links[0][0]), link_cpu_us=int(links[0][1]),
                            binary_bytes=int(stored[0][0]), binary_format=stored[0][1]))
    finals = re.findall(r'SWEET PROGRAM CACHE PRIME END result=0 compiled=(\d+) failed=0 max_entries=8 original_requests=1', after)
    assert len(finals) == 1 and int(finals[0]) == len(samples)
    assert 'SWEET PROGRAM CACHE PRIME caller result=0 compiled='+finals[0]+' failed=0 full_native_preverified=1' in after
    assert after.index('SWEET AIR STATIC HALF END result=0') < begins[0].start() < after.index('SWEET METAL PROBES END process=backboardd result=0')
    return dict(original_token=original['token'], original_raw_sha256=original['log_sha256'], prime_token=prime['token'],
                prime_raw_sha256=prime['log_sha256'], kernel_sha256=native['kernel_sha256'],
                GLES_sha256=native['gles_library_sha256'], Metal_sha256=native['metal_library_sha256'],
                source_map_sha256=native['metal_source_files_sha256'], complete_native_groups=len(native['native_tests']),
                samples=samples, return_seconds_with_health=prime['return_seconds_with_health'],
                original_failure_preserved=True, full_UI_not_inferred=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('original_directory')
    parser.add_argument('prime_directory')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    report = verify(args.original_directory, args.prime_directory)
    out = Path(args.output)
    assert not out.exists()
    out.write_bytes((json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps(report))
