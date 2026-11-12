"""Independently check the two real ownership snapshots and identity-root hash."""
import re


def only(pattern, text):
    matches = list(re.finditer(pattern, text))
    assert len(matches) == 1, pattern
    return matches[0]


def word_hash(words):
    value = 14695981039346656037
    for word in words:
        for byte in word.to_bytes(8, 'little'):
            value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return f'{value:016x}'


def verify(text):
    snapshots = []
    number = r'(0x[0-9a-f]+|0)'
    for snap in (1, 2):
        n = str(snap)
        begin = only(r'SWEET SG OWN BEGIN snap=' + n + r' post_live=1 observer_only=1 root_updates=0 at_ms=(\d+)', text)
        end = only(r'SWEET SG OWN END snap=' + n + r' result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1 root_updates=0 alias_work=0 at_ms=(\d+)', text)
        context = only(r'SWEET SG OWN CONTEXT snap=' + n + r' id0=' + number + r' id1=' + number + r' id2=' + number + r' scr0=' + number + r' cbar=' + number + r' cba2r=' + number + r' sctlr=' + number + r' tcr=' + number + r' tcr2=' + number + r' mair=' + number, text)
        fields = [int(v, 0) for v in context.groups()]
        assert fields[0] & 0x40000000 and fields[1] == 0x20008003
        assert fields[4:8] == [0x1f300, 1, 0x1167, 0x80201c] and fields[9] == 0x44
        assert fields[8] & 7 in (1, 2)
        assert int(end[1]) == [32, 36, 40, 42, 44, 48][fields[8] & 7]
        observed = only(r'SWEET SG OWN ROOT snap=' + n + r' expected=' + number + r' ttbr0=' + number + r' after=' + number + r' ttbr1=' + number + r' words=512 nonzero=6 hash=([0-9a-f]{16})', text)
        expected, ttbr, after, upper = [int(observed[i], 0) for i in (1, 2, 3, 4)]
        assert expected == ttbr == after and upper == 0
        assert 0x9f800000 <= expected < 0xfaf00000 and expected % 4096 == 0
        words = list(re.finditer(r'SWEET SG OWN WORD snap=' + n + r' index=(\d+) value=' + number, text))
        assert len(words) == 8 and [int(w[1]) for w in words] == list(range(8))
        actual = [int(w[2], 0) for w in words]
        assert actual[0] == actual[1] == 0
        for i in range(2, 8):
            value = actual[i]
            assert value & ~(0xfff | (3 << 53)) == i << 30 and value & 3 == 1
            assert (value >> 2) & 7 == 0 and value & (1 << 6) and not value & (1 << 7)
            assert (value >> 8) & 3 == 2 and value & (1 << 10)
        computed = word_hash(actual + [0] * 504)
        assert computed == observed[5]
        streams = list(re.finditer(r'SWEET SG OWN STREAM snap=' + n + r' slot=(\d+) smr=' + number + r' s2cr=' + number, text))
        assert len(streams) == 4 and [int(s[1]) for s in streams] == list(range(4))
        route = [{'slot': int(s[1]), 'smr': int(s[2], 0), 's2cr': int(s[3], 0)} for s in streams]
        assert route[0]['smr'] == 0x80000000 and route[0]['s2cr'] == 0
        for stream in route[1:]:
            match = stream['smr']; mask = (match >> 16) & 0x7fff; sid = match & 0x7fff
            assert not (match & 0x80000000) or sid & ~mask
        state = only(r'SWEET SG OWN STATE snap=' + n + r' trust=0 cp=' + number + r' seq=(\d+) done=(\d+) rptr=(\d+) wptr=(\d+) fsr=' + number + r' tlb_status=' + number, text)
        cp_bits = int(state[1], 0)
        seq, done, read, write = [int(state[i]) for i in (2, 3, 4, 5)]
        assert cp_bits & 3 == 3 and seq and done
        assert (done - seq) & 0xffffffff < 0x80000000 and read == write
        assert not (int(state[7], 0) & 1)
        assert begin.start() < context.start() < observed.start() < words[0].start() < streams[0].start() < state.start() < end.start()
        assert int(end[2]) >= int(begin[1])
        snapshots.append({'snapshot': snap, 'root_pa': expected, 'root_hash': computed,
                          'root_words': actual, 'root_hash_independently_recomputed': True,
                          'streams': route, 'raw_tcr2': fields[8], 'sequence': seq,
                          'completed': done, 'observed_fsr': int(state[6], 0),
                          'observer_root_updates': 0, 'alias_gpu_work': 0})
    for key in ('root_pa', 'root_hash', 'streams', 'raw_tcr2'):
        assert snapshots[0][key] == snapshots[1][key]
    return snapshots
