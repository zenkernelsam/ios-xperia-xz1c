"""Check one logical RAM library read without erasing an explicit CDC replay."""
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
    for i, begin in enumerate(positions):
        end = positions[i + 1] if i + 1 < len(positions) else ack
        segment = text[begin:end]
        assert segment.count(library_line) == 1, 'RAM library read missing or repeated inside one prefix'
        records = re.findall(r'SWEET GLRAM: library[^\r\n]*', segment)
        assert records == [library_line], 'RAM library read returned unexpected bytes/hash/status'
        offset = begin + segment.index(library_line)
        reads.append(offset)
        blocks.append(raw[begin:offset + len(library_line)])
    if len(positions) == 2:
        # serial_worker explicitly requests one replay on the current CDC.
        # Require the command's retained-log marker, one kernel banner, and an
        # exactly identical READY-to-read byte block. Distinct prefixes, another
        # boot token, or a changed read remain failures; raw bytes stay untouched.
        assert text[:live].count('VERMEER DEBUG: replay retained RAM log') == 1
        assert text[:live].count('Darwin Kernel Version') == 1
        assert all(offset < live for offset in reads), 'An active read cannot be treated as a startup replay'
        assert blocks[0] == blocks[1], 'Repeated startup blocks differ; not an exact replay'
    return {'logical_library_reads': 1, 'raw_library_reads': len(reads),
            'ready_positions': positions, 'read_positions': reads,
            'startup_block_sha256': hashlib.sha256(blocks[-1]).hexdigest(),
            'explicit_exact_prefix_replay': len(positions) == 2,
            'raw_log_unchanged': True}
