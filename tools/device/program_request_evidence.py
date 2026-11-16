"""Reassemble bounded, byte-exact read-only requests from a current boot."""
import argparse
import hashlib
import json
import os
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from program_request import CONFIG_BYTES, EXPORT_LIMIT, FRAGMENT_BYTES, VERTEX_BYTES


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode_record(record, spec):
    assert len(record) >= 136 and record[:8] == b'SGPB0001'
    revision, kind, fmt, payload, vs, fs, config_bytes, reserved = struct.unpack_from('<8I', record, 8)
    assert (revision, kind, fmt, config_bytes, reserved) == (1, 1, 0, CONFIG_BYTES, 0)
    assert vs == VERTEX_BYTES and fs in FRAGMENT_BYTES
    assert payload == CONFIG_BYTES + vs + fs and len(record) == payload + 136
    assert record[40:72].hex() == spec['context_identity']
    data = record[136:]
    assert sha(data) == record[104:136].hex()
    config, vertex, fragment = data[:CONFIG_BYTES], data[CONFIG_BYTES:CONFIG_BYTES+vs], data[CONFIG_BYTES+vs:]
    assert b'\0' not in vertex + fragment
    key = sha(record[40:72] + struct.pack('<III', vs, fs, CONFIG_BYTES) + config + vertex + fragment)
    assert key == record[72:104].hex()
    return dict(record_sha256=sha(record), key=key, record_bytes=len(record),
                vertex_bytes=vs, fragment_bytes=fs, vertex_sha256=sha(vertex),
                fragment_sha256=sha(fragment), config_sha256=sha(config))


def extract(text, token, spec):
    live = 'SWEET LOOP LIVE proto=1 token=' + token
    ack = 'SWEET LOOP ACK FASTBOOT token=' + token
    assert text.count(live) == 1 and text.count(ack) == 1
    scope = text[text.index(live):text.index(ack)]
    scope = re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)', '', scope)
    framing = spec.get('framing_revision', 1)
    assert framing in (1, 2)
    if framing == 2:
        starts = list(re.finditer(r'SWEET PROGRAM REQUEST begin index=(\d+) bytes=(\d+) vertex_bytes=(\d+) fragment_bytes=(\d+) read_only=1 chunks=32', scope))
    else:
        starts = list(re.finditer(r'SWEET PROGRAM REQUEST begin index=(\d+) bytes=(\d+) record_sha=([0-9a-f]{64}) identity=([0-9a-f]{64}) key=([0-9a-f]{64}) vertex_bytes=(\d+) fragment_bytes=(\d+) read_only=1', scope))
    assert len(starts) == 2
    records, metadata, chunk_count = [], [], 0
    for index, start in enumerate(starts):
        fields = start.groups()
        assert int(fields[0]) == index and int(fields[1]) <= 131072
        end_pattern = r'SWEET PROGRAM REQUEST END index=' + str(index) + r' bytes=(\d+) record_sha=([0-9a-f]{64}) complete=1 read_only=1'
        ends = list(re.finditer(end_pattern, scope))
        assert len(ends) == 1 and ends[0].start() > start.end()
        end = ends[0]
        if index + 1 < len(starts):
            assert end.end() < starts[index+1].start()
        block = scope[start.end():end.start()]
        if framing == 2:
            def field(label, key):
                found = re.findall(r'SWEET PROGRAM REQUEST '+label+' index='+str(index)+' '+key+r'=([0-9a-f]{64})', block)
                assert len(found) == 1
                return found[0]
            fields = (fields[0], fields[1], field('digest', 'record_sha'), field('identity', 'value'), field('key', 'value'), fields[2], fields[3])
        assert fields[3] == spec['context_identity'] and end.groups() == (fields[1], fields[2])
        chunks = re.findall(r'SWEET PROGRAM REQUEST data index=(\d+) offset=(\d+) bytes=(\d+) hex=([0-9a-f]+)', block)
        chunk_count += len(chunks)
        record = bytearray()
        for chunk_index, offset, count, hex_bytes in chunks:
            assert int(chunk_index) == index and int(offset) == len(record)
            decoded = bytes.fromhex(hex_bytes)
            assert 0 < int(count) <= (32 if framing == 2 else 128) and len(decoded) == int(count)
            assert len(record) + len(decoded) <= int(fields[1])
            record.extend(decoded)
        record = bytes(record)
        assert len(record) == int(fields[1]) and sha(record) == fields[2]
        info = decode_record(record, spec)
        assert info['key'] == fields[4] and info['vertex_bytes'] == int(fields[5]) and info['fragment_bytes'] == int(fields[6])
        records.append(record)
        metadata.append(info)
    finals = list(re.finditer(r'SWEET PROGRAM REQUEST EXPORT END result=0 records=2 bytes=(\d+) limit=262144 read_only=1 no_compile=1', scope))
    assert len(finals) == 1 and finals[0].start() > end.end()
    assert sum(map(len, records)) == int(finals[0].group(1)) <= EXPORT_LIMIT
    assert sorted(row['fragment_bytes'] for row in metadata) == sorted(FRAGMENT_BYTES)
    assert scope.count('SWEET PROGRAM REQUEST begin ') == 2 and scope.count('SWEET PROGRAM REQUEST END ') == 2
    assert scope.count('SWEET PROGRAM REQUEST data ') == chunk_count
    if framing == 2:
        for marker in ('identity', 'key', 'digest'):
            assert scope.count('SWEET PROGRAM REQUEST '+marker+' ') == 2
    assert 'SWEET PROGRAM CACHE prime_begin' not in scope
    return records, dict(complete_samples=1, incomplete_samples=0, records=metadata,
                         bytes_checked=sum(map(len, records)), read_only=True, no_compile=True)


def verify(text, token, spec):
    return extract(text, token, spec)[1]


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('directory')
    parser.add_argument('--original-directory', required=True)
    parser.add_argument('--output-directory', required=True)
    args = parser.parse_args()
    from trial_evidence import verify as verify_trial
    current = verify_trial(args.directory, 'gpu')
    root = Path(args.directory)
    row = json.loads((root/'results.json').read_bytes())[-1]['artifact']
    old_root = Path(args.original_directory)
    old_row = json.loads((old_root/'results.json').read_bytes())[-1]['artifact']
    assert row['kernel_sha256'] == old_row['kernel_sha256'] and row['metal_resources'] == old_row['metal_resources']
    old = (old_root/'cycle-01/serial.raw.log').read_text(encoding='ascii', errors='replace')
    old_scope = old[old.index('SWEET LOOP LIVE proto=1 token='+old_row['token']):old.index('SWEET LOOP ACK FASTBOOT token='+old_row['token'])]
    for fs in FRAGMENT_BYTES:
        assert re.search(r'SWEET PROGRAM CACHE request result=0 vertex_bytes=4840 fragment_bytes='+str(fs)+r' immutable=1', old_scope)
    records, report = extract((root/'cycle-01/serial.raw.log').read_text(encoding='ascii', errors='replace'), current['token'], row['exception_observer']['program_request_export'])
    out = Path(args.output_directory).resolve()
    assert out.is_relative_to(Path(os.environ['TEMP']).resolve()) and not out.exists()
    out.mkdir()
    for record, info in zip(records, report['records']):
        stem = str(info['fragment_bytes'])
        (out/(stem+'.req')).write_bytes(record)
        (out/(stem+'.vert')).write_bytes(record[136+CONFIG_BYTES:136+CONFIG_BYTES+VERTEX_BYTES])
        (out/(stem+'.frag')).write_bytes(record[136+CONFIG_BYTES+VERTEX_BYTES:])
    report.update(original_token=old_row['token'], export_token=current['token'], export_raw_sha256=current['log_sha256'],
                  source_values_from_actual_runtime=True, full_UI_not_inferred=True)
    (out/'manifest.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps(report))
