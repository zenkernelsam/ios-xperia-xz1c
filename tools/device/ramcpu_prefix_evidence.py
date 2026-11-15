"""Check library reads and byte-exact complete or partial CDC startup replays."""
import hashlib
import re


def verify(raw, token, library_line):
    assert isinstance(raw, bytes), 'Original raw log bytes are required'
    text = raw.decode('ascii', 'replace')
    ready = 'SWEET LOOP READY proto=1 token=' + token
    live = text.index('SWEET LOOP LIVE proto=1 token=' + token)
    ack = text.index('SWEET LOOP ACK FASTBOOT token=' + token)
    positions = [m.start() for m in re.finditer(re.escape(ready), text[:live])]
    assert len(positions) in (1, 2), 'Unexpected number of current-boot READY prefixes'
    tokens = re.findall(r'SWEET LOOP READY proto=1 token=([0-9a-f]{16})', text[:live])
    assert tokens == [token] * len(positions), 'Foreign boot prefix in the capture'
    assert positions[-1] < live < ack
    blocks = []
    reads = []
    records_by_prefix = []
    for i, begin in enumerate(positions):
        end = positions[i + 1] if i + 1 < len(positions) else ack
        segment = text[begin:end]
        records = re.findall(r'SWEET GLRAM: library[^\r\n]*', segment)
        records_by_prefix.append(records)
        assert records in ([], [library_line]), 'RAM library read returned unexpected bytes/hash/status'
        if records:
            offset = begin + segment.index(library_line)
            reads.append(offset)
            blocks.append(raw[begin:offset + len(library_line)])
    partial = None
    if len(positions) == 2:
        # serial_worker explicitly requests one replay on the current CDC.
        # Require the command's retained-log marker, one kernel banner, and an
        # exactly identical READY-to-read byte block. Distinct prefixes, another
        # boot token, or a changed read remain failures; raw bytes stay untouched.
        assert text[:live].count('VERMEER DEBUG: replay retained RAM log') == 1
        assert text[:live].count('Darwin Kernel Version') == 1
        if records_by_prefix == [[library_line], [library_line]]:
            assert all(offset < live for offset in reads), 'An active read cannot be treated as a startup replay'
            assert blocks[0] == blocks[1], 'Repeated startup blocks differ; not an exact replay'
        else:
            # A retained prefix can end before the mount/read has finished.
            # The actual read after LIVE then occurs only once. Prove every
            # replayed byte, its complete line ending and the command boundary;
            # do not count this live read as another startup replay.
            assert records_by_prefix == [[], [library_line]] and reads[0] > live
            marker = text.index('VERMEER DEBUG: replay retained RAM log', positions[1], live)
            partial = raw[positions[1]:marker]
            assert len(ready) < len(partial) < positions[1] - positions[0]
            assert partial.endswith(b'\n') and b'SWEET GLRAM: library' not in partial
            assert raw[positions[0]:positions[0] + len(partial)] == partial, 'Partial retained startup bytes differ'
            suffix = raw[marker + len('VERMEER DEBUG: replay retained RAM log'):live]
            assert suffix in (b'\n', b'\n\n', b'\r\n', b'\r\n\r\n'), 'Unexpected records between replay marker and LIVE'
    else:
        assert records_by_prefix == [[library_line]], 'RAM library read missing from the current boot'
    result = {'logical_library_reads': 1, 'raw_library_reads': len(reads),
            'ready_positions': positions, 'read_positions': reads,
            'startup_block_sha256': hashlib.sha256(blocks[-1]).hexdigest(),
            'explicit_exact_prefix_replay': len(reads) == 2,
            'raw_log_unchanged': True}
    if partial is not None:
        result.update(explicit_exact_partial_prefix_replay=True, partial_replay_bytes=len(partial),
                      partial_replay_sha256=hashlib.sha256(partial).hexdigest(), active_read_not_counted_as_replay=True)
    return result
