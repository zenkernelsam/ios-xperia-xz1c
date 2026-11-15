"""Bind a background prime to the original failed shader and complete GPU proof."""
import argparse
import json
import re
from pathlib import Path
from failed_trial_evidence import verify as verify_failure
from trial_evidence import verify as verify_trial
from native_prerequisite import verify as verify_native


def scope(directory, token):
    raw = (Path(directory) / 'cycle-01/serial.raw.log').read_bytes()
    text = raw.decode('ascii', 'replace')
    return text[text.index('SWEET LOOP LIVE proto=1 token=' + token):
                text.index('SWEET LOOP ACK FASTBOOT token=' + token)]


def verify(original_directory, prime_directory):
    original = verify_failure(original_directory)
    assert original['original_reason'] == 'EXTENSION_CHECKIN_TIMEOUT'
    assert original['original_extension_checkin_timeout_count'] >= 1
    current = verify_trial(prime_directory, 'gpu', original['kernel_sha256'], original['version'])
    native = verify_native(prime_directory)
    old_row = json.loads((Path(original_directory) / 'results.json').read_bytes())[-1]['artifact']
    row = json.loads((Path(prime_directory) / 'results.json').read_bytes())[-1]['artifact']
    assert row['exception_observer'].get('program_cache_prime')
    assert row['metal_resources'] == old_row['metal_resources'], 'Original and primer libraries/source map differ'
    assert 'Program_cache' in native['native_tests'] and len(native['native_tests']) == 13
    before = scope(original_directory, original['token'])
    after = scope(prime_directory, current['token'])
    sources = {}
    for stage in ('vertex', 'fragment'):
        entries = set(re.findall(r'SWEET AIR RENDER compile_begin stage=' + stage + r' bytes=(\d+) hash=([0-9a-f]{16})', before))
        assert len(entries) == 1, 'Original first-pipeline source is ambiguous'
        size, digest = next(iter(entries))
        sources[stage] = {'bytes': int(size), 'fnv64': digest}
    vs, fs = sources['vertex']['bytes'], sources['fragment']['bytes']
    assert re.search(r'SWEET PROGRAM CACHE request result=0 vertex_bytes=' + str(vs) + r' fragment_bytes=' + str(fs) + r' immutable=1', before)
    assert not re.search(r'SWEET AIR RENDER compile_end stage=link[^\r\n]*status=1', before)
    begins = list(re.finditer(r'SWEET PROGRAM CACHE prime_begin vertex_bytes=' + str(vs) + r' fragment_bytes=' + str(fs) + r' source_verified=1', after))
    assert len(begins) == 1
    start = begins[0].start()
    end = after.index('SWEET PROGRAM CACHE prime_entry result=0 cache_state=2 reason=', start)
    block = after[start:end]
    for stage, source in sources.items():
        assert 'SWEET AIR RENDER compile_begin stage=' + stage + ' bytes=' + str(source['bytes']) + ' hash=' + source['fnv64'] in block
    links = re.findall(r'SWEET AIR RENDER compile_end stage=link wall_us=(\d+) cpu_us=(\d+) cpu_clock=1 status=1', block)
    stored = re.findall(r'SWEET PROGRAM CACHE stored result=0 bytes=(\d+) format=(0x[0-9a-f]+) error=0 exact_build=1', block)
    assert len(links) == len(stored) == 1 and 0 < int(stored[0][0]) <= 8 << 20
    assert 'vertex_hash=' + sources['vertex']['fnv64'] + ' fragment_hash=' + sources['fragment']['fnv64'] + ' linked=1 robust=1' in block
    final = re.findall(r'SWEET PROGRAM CACHE PRIME END result=0 compiled=(\d+) failed=0 max_entries=8 original_requests=1', after)
    assert len(final) == 1 and int(final[0]) >= 1
    assert 'SWEET PROGRAM CACHE PRIME caller result=0 compiled=' + final[0] + ' failed=0 full_native_preverified=1' in after
    assert after.index('SWEET AIR SAMPLER END result=0') < start < end < after.index('SWEET METAL PROBES END process=backboardd result=0')
    return {'original_run_token': original['token'], 'original_raw_sha256': original['log_sha256'],
            'prime_run_token': current['token'], 'prime_raw_sha256': current['log_sha256'],
            'kernel_sha256': native['kernel_sha256'], 'GLES_sha256': native['gles_library_sha256'],
            'Metal_sha256': native['metal_library_sha256'], 'native_source_map_sha256': native['metal_source_files_sha256'],
            'actual_original_sources': sources, 'cold_link_wall_us': int(links[0][0]), 'cold_link_cpu_us': int(links[0][1]),
            'stored_binary_bytes': int(stored[0][0]), 'stored_binary_format': stored[0][1], 'complete_native_groups': 13,
            'return_seconds_with_health': current['return_seconds_with_health'], 'original_failure_unchanged': True,
            'full_UI_or_original_cache_hit_not_inferred': True}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('original_directory'); parser.add_argument('prime_directory'); parser.add_argument('--output', required=True)
    args = parser.parse_args(); result = verify(args.original_directory, args.prime_directory)
    output = Path(args.output); assert not output.exists()
    output.write_bytes((json.dumps(result, indent=2) + '\n').encode())
    print(json.dumps(result, indent=2))
