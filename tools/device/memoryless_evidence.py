"""Original Memoryless pass pages, byte recipe, guards and queue isolation."""
import re
import struct

BEGIN = 'SWEET MEMORYLESS BEGIN phases=8 width=17 height=9 color_bytes=1280 depth_bytes=1280 stencil_bytes=704 public_CPU_backing=0'
PAGE = re.compile(r'SWEET MEMORYLESS PAGE phase=(\d+) slot=(\d+) pass=(\d+) public=([0-9a-f]{16}) handle=(\d+) GPU=(0x[0-9a-f]+) bytes=(\d+) pitch=(\d+) offset=(\d+) format=(\d+)')
PHASE = re.compile(r'SWEET MEMORYLESS PHASE index=(\d+) valid=([01]) sequence=(\d+) operations=(\d+) checked_bytes=(\d+) mismatches=(\d+) original_pass_pages=1')
HASH = re.compile(r'SWEET MEMORYLESS HASH index=(\d+) color=([0-9a-f]{16}) depth=([0-9a-f]{16}) stencil=([0-9a-f]{16})')
PUBLIC = re.compile(r'SWEET MEMORYLESS PUBLIC valid=([01]) allocated=(\d+) depth=([0-9a-f]{16}) stencil=([0-9a-f]{16}) color=([0-9a-f]{16}) backing_exposed=0')
END = re.compile(r'SWEET MEMORYLESS END result=(\d+) phases=(\d+) checked_bytes=19456 guard_bytes=10888 final_allocated=(\d+) GPU_only=1 transient_pages=1')
SLOTS = ((8,), (8,), (9,), (8, 9), (0,), (0,), (8, 9), (8, 9))
OPERATIONS = (2, 1, 2, 2, 1, 1, 1, 1)


def fnv(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return f'{value:016x}'


def expected(phase):
    assert 0 <= phase < 8
    color = bytearray([0xa5] * 1280)
    depth = bytearray([0xb6] * 1280)
    stencil = bytearray([0xc7] * 704)
    accepted = phase not in (1, 7)
    for y in range(9):
        for x in range(17):
            box = 3 <= x < 14 and 2 <= y < 7
            at = 64 + y * 128 + x * 4
            color[at:at + 4] = bytes((255, 255, 0, 255) if phase == 5 else (191, 128, 64, 255) if box and accepted else (255, 0, 255, 255))
            depth[at:at + 4] = struct.pack('<f', 0.125 if phase in (1, 7) else 0.25 if box else 0.75)
            value = (18 if box else 17) if phase == 2 else (126 if box else 129) if phase == 7 else (43 if phase == 3 else 42) if box else (127 if phase == 3 else 17)
            stencil[64 + y * 64 + x] = value
    slots = SLOTS[phase]
    return fnv(color), fnv(depth) if 8 in slots else '0' * 16, fnv(stencil) if 9 in slots else '0' * 16


def verify(text, token, fixture,tiled_revision=0):
    assert tiled_revision in (0,2,3,4,5)
    live = 'SWEET LOOP LIVE proto=1 token=' + token
    ack = 'SWEET LOOP ACK FASTBOOT token=' + token
    assert text.count(live) == text.count(ack) == 1
    scope = text[text.index(live):text.index(ack)]
    assert scope.count(BEGIN) == scope.count('SWEET MEMORYLESS BEGIN ') == 1
    block = scope[scope.index(BEGIN):]
    ends = list(END.finditer(block))
    assert len(ends) == 1 and ends[0].groups() == ('0', '8', '0'), 'Original transient operation or retirement failed'
    block = block[:ends[0].start()]
    assert block.count('SWEET MEMORYLESS INPUT fixture_sha256=' + fixture['library_sha256']) == 1
    public = list(PUBLIC.finditer(block))
    assert len(public) == 1 and (public[0][1], public[0][2]) == ('1', '0')
    identities = {8: public[0][3], 9: public[0][4], 0: public[0][5]}
    assert len(set(identities.values())) == 3 and all(int(value, 16) for value in identities.values())
    phases, hashes, pages = list(PHASE.finditer(block)), list(HASH.finditer(block)), list(PAGE.finditer(block))
    assert len(phases) == len(hashes) == 8 and len(pages) == 11
    assert block.count('SWEET MEMORYLESS PHASE ') == block.count('SWEET MEMORYLESS HASH ') == 8
    assert block.count('SWEET MEMORYLESS PAGE ') == len(pages)
    assert block.count('SWEET MEMORYLESS ') == 1 + 1 + len(pages) + len(phases) + len(hashes) + 1 + 1, 'Unknown or truncated transient record'
    assert [int(p[1]) for p in phases] == [int(h[1]) for h in hashes] == list(range(8))
    sequences, records, ids = [], [], [];tiled=[]
    page_info = {}
    for phase, (p, h) in enumerate(zip(phases, hashes)):
        slots = SLOTS[phase]
        checked = 1280 + (1280 if 8 in slots else 0) + (704 if 9 in slots else 0)
        assert p[2] == '1' and int(p[4]) == OPERATIONS[phase] and int(p[5]) == checked and p[6] == '0'
        sequence = int(p[3]); assert sequence > 0 and sequence not in sequences
        assert (h[2], h[3], h[4]) == expected(phase), f'Original transient bytes differ at phase {phase}'
        assert p.start() < h.start() and (phase == 7 or h.end() < phases[phase + 1].start())
        original = [page for page in pages if int(page[1]) == phase]
        assert tuple(int(page[2]) for page in original) == slots
        for page in original:
            slot = int(page[2]); identity, handle, gpu = int(page[3]), int(page[5]), int(page[6], 16)
            assert identity > 0 and handle > 0 and page[4] == identities[slot] and 0x40000000 <= gpu < 0x80000000 and gpu % 16384 == 0
            assert (int(page[7]), int(page[8]), int(page[9]), int(page[10])) == ((704, 64, 64, 253) if slot == 9 else (1280, 128, 64, 252 if slot == 8 else 80))
            assert page.end() < p.start()
            key = (phase, slot); page_info[key] = dict(pass_id=identity, handle=handle, gpu_address=page[6], public_identity=page[4])
        assert len({int(page[3]) for page in original}) == 1
        identity = int(original[0][3]); assert identity not in ids; ids.append(identity)
        # Every depth/stencil page must be imported and retired by the actual
        # production backend inside this phase, before its checked result.
        start = min(page.start() for page in original)
        segment = block[start:p.start()]
        if 0 in slots:
            info = page_info[phase, 0]
            imported = list(re.finditer(r'SWEET AIR RENDER texture handle=(\d+) width=17 height=9 pitch=128 format=80 offset=64 gpu=(0x[0-9a-f]+) original_pages=1', segment))
            retired = list(re.finditer(r'SWEET METAL MRT retired index=0 handle=(\d+) format=80 result=0 sequence=(\d+) original_pages=1', segment))
            imported = [m for m in imported if int(m[1]) == info['handle']]
            retired = [m for m in retired if int(m[1]) == info['handle']]
            assert len(imported) == len(retired) == OPERATIONS[phase]
            assert all(m[2] == info['gpu_address'] for m in imported)
            assert all(0 < int(m[2]) <= sequence for m in retired)
        if tiled_revision and any(slot in (8,9) for slot in slots):
            from depth_tiled_evidence import plane,verify_groups
            groups=[[plane(page_info[phase,slot]['handle'],page_info[phase,slot]['gpu_address'],
                           253 if slot==9 else 252,1,64 if slot==9 else 128,2 if operation==0 else 1)
                     for slot in slots if slot in (8,9)] for operation in range(OPERATIONS[phase])]
            tiled.append(dict(phase=phase,**verify_groups(segment,groups,tiled_revision)))
        for slot, name in ((8, 'DEPTH'), (9, 'STENCIL')):
            if slot not in slots:
                continue
            info = page_info[phase, slot]
            pitch = 64 if slot == 9 else 128
            imported = list(re.finditer(r'SWEET METAL ' + name + r' import handle=(\d+) width=17 height=9 pitch=' + str(pitch) + r' offset=64 GPU=(0x[0-9a-f]+) original_pages=1', segment))
            retired = list(re.finditer(r'SWEET METAL ' + name + r' retired handle=(\d+) result=0 sequence=(\d+) original_pages=1', segment))
            # Two queued concurrent passes can interleave. Match actual page,
            # not whichever generic import happened last in the process.
            imported = [m for m in imported if int(m[1]) == info['handle']]
            retired = [m for m in retired if int(m[1]) == info['handle']]
            assert len(imported) == len(retired) == OPERATIONS[phase]
            assert all(m[2] == info['gpu_address'] for m in imported)
            assert all(0 < int(m[2]) <= sequence for m in retired)
        sequences.append(sequence)
        records.append(dict(phase=phase, sequence=sequence, operations=OPERATIONS[phase], checked_bytes=checked, color_hash=h[2], depth_hash=h[3], stencil_hash=h[4]))
    assert sequences[:6] == sorted(sequences[:6])
    assert all(sequence > sequences[5] for sequence in sequences[6:])
    assert block.count('SWEET MEMORYLESS ISOLATION valid=1 two_queues=1 encoders_materialized_before_commit=1 different_depth_and_stencil_pages=1') == 1
    for slot in (8, 9):
        assert page_info[6, slot]['handle'] != page_info[7, slot]['handle']
        assert page_info[6, slot]['gpu_address'] != page_info[7, slot]['gpu_address']
    result=dict(complete_samples=1, incomplete_samples=0, phases=8, checked_bytes=19456, guard_bytes=10888,
        records=records, original_pass_pages=[dict(phase=phase, slot=slot, **value) for (phase, slot), value in page_info.items()],
        exact_author_pixels_original_private_pass_storage_and_guards_verified=True,
        two_queue_pages_are_independent=True, public_CPU_buffer_not_exposed=True, final_allocated=0,
        raw_is_actual_device_evidence_only_when_called_from_verified_trial=True)
    if tiled_revision:result['tiled_work_planes']=tiled
    return result
