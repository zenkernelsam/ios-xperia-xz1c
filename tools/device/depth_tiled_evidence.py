"""Bind actual FDL5 work planes, GPU copies and retirement to authored pages."""
import re

PAGE = re.compile(r'SWEET DEPTH TILE PAGE original=(\d+) work=(\d+) format=(\d+) samples=(\d+) pitch=(\d+) bytes=(\d+) tile=(\d+) original_pitch=(\d+) original_offset=(\d+)')
MAP = re.compile(r'SWEET DEPTH TILE MAP original=(\d+) work=(\d+) original_GPU=(0x[0-9a-f]+) work_GPU=(0x[0-9a-f]+) load=(\d+) width=(\d+) height=(\d+) cpp=(\d+) raw_cpp=(\d+) alias_samples=1')
COPY = re.compile(r'SWEET DEPTH TILE COPY original=(\d+) work=(\d+) direction=(load|store) samples=(\d+) width=(\d+) height=(\d+) result=0 sequence=(\d+) GPU_only=1 fence=1')
PAIR = re.compile(r'SWEET DEPTH TILE PAIR depth=(\d+) stencil=(\d+) depth_work=(\d+) stencil_work=(\d+) samples=(\d+) result=0 separate_z32s8=1')
RETIRE = re.compile(r'SWEET DEPTH TILE RETIRE original=(\d+) work=(\d+) result=0 references_retired=1')
BLIT = re.compile(r'SWEET DEPTH TILE BLIT original=(\d+) work=(\d+) direction=(load|store) gen=(\d+) src_fmt=(\d+) dst_fmt=(\d+) src_color=(\d+) dst_color=(\d+) src_samples=(\d+) dst_samples=(\d+) src_mask=(\d+) dst_mask=(\d+) mask=(\d+) filter=(\d+) src_tile=(\d+) dst_tile=(\d+)')
BOX = re.compile(r'SWEET DEPTH TILE BOX original=(\d+) work=(\d+) src=(-?\d+,-?\d+,-?\d+,-?\d+,-?\d+,-?\d+) dst=(-?\d+,-?\d+,-?\d+,-?\d+,-?\d+,-?\d+) src_size=(\d+,\d+,\d+) dst_size=(\d+,\d+,\d+) src_pitch=(\d+) dst_pitch=(\d+) src_offset=(\d+) dst_offset=(\d+)')


def plane(handle, gpu, format_, samples, pitch, load, publish=True):
    return dict(handle=int(handle), gpu=int(gpu, 16) if isinstance(gpu, str) else gpu,
                format=format_, samples=samples, pitch=pitch, offset=64, width=17, height=9,
                load=load, publish=publish)


def layout_bytes(format_, samples, width=17, height=9):
    # Independent arithmetic from pinned FDL5: cpp1/2 use 128-pixel pitch,
    # all other cpp use 64; tiled height is 32 for cpp1, otherwise16. The
    # single layer is page-aligned to4096, including tiny attachment probes.
    cpp = (1 if format_ == 253 else 4) * samples
    pitch = ((width + (127 if cpp < 4 else 63)) // (128 if cpp < 4 else 64)) * (128 if cpp < 4 else 64) * cpp
    rows = ((height + (31 if cpp == 1 else 15)) // (32 if cpp == 1 else 16)) * (32 if cpp == 1 else 16)
    return cpp, pitch, ((pitch * rows + 4095) // 4096) * 4096


def verify_groups(segment, groups,revision=2):
    """Each group is one backend operation's ordered depth/stencil planes.

    Filtered segments for concurrent probes retain original handles explicitly;
    no unrelated plane is used as proof of this group's work or copy.
    """
    assert revision in (2,3)
    handles = {p['handle'] for group in groups for p in group}
    tables = [list(pattern.finditer(segment)) for pattern in (PAGE, MAP, COPY, PAIR, RETIRE)]
    pages, maps, copies, pairs, retired = tables
    for pattern, rows, name in zip((PAGE, MAP, COPY, PAIR, RETIRE), tables, ('PAGE', 'MAP', 'COPY', 'PAIR', 'RETIRE')):
        assert segment.count('SWEET DEPTH TILE '+name+' ') == len(rows), 'Malformed or failed tiled '+name+' record'
    expected = [p for group in groups for p in group]
    assert all(p['format'] in (252,253) and p['samples'] in (1,2,4) and p['load'] in (0,1,2) for p in expected)
    pages = [m for m in pages if int(m[1]) in handles]
    maps = [m for m in maps if int(m[1]) in handles]
    copies = [m for m in copies if int(m[1]) in handles]
    retired = [m for m in retired if int(m[1]) in handles]
    pairs = [m for m in pairs if int(m[1]) in handles or int(m[2]) in handles]
    blits = [m for m in BLIT.finditer(segment) if int(m[1]) in handles]
    boxes = [m for m in BOX.finditer(segment) if int(m[1]) in handles]
    if revision==3:
        assert 'SWEET DEPTH TILE REJECT ' not in segment
        assert segment.count('SWEET DEPTH TILE BLIT ')==len(list(BLIT.finditer(segment)))
        assert segment.count('SWEET DEPTH TILE BOX ')==len(list(BOX.finditer(segment)))
        assert len(blits)==len(boxes)==len(copies), 'Original raw blit/box metadata missing'
    assert len(pages) == len(maps) == len(retired) == len(expected), 'Work page/map/retirement count differs'
    assert len(copies) == sum((p['load'] == 1) + p['publish'] for p in expected)
    assert len(pairs) == sum(any(p['format'] == 253 for p in group) for group in groups)
    index = 0; records = []
    for group in groups:
        own_pages = pages[index:index+len(group)]
        begin = own_pages[0].start()
        following = index + len(group)
        end = pages[following].start() if following < len(pages) else len(segment)
        work = {}; work_gpus = {}; spans = []
        for p, page in zip(group, own_pages):
            original, handle, format_, samples, pitch, size, tile, original_pitch, offset = map(int, page.groups())
            cpp, expected_pitch, expected_size = layout_bytes(p['format'], p['samples'], p['width'], p['height'])
            assert (original, format_, samples, original_pitch, offset) == (p['handle'], p['format'], p['samples'], p['pitch'], p['offset'])
            assert handle > 0 and handle not in handles and (pitch, size, tile) == (expected_pitch, expected_size, 2)
            mapping = [m for m in maps if begin <= m.start() < end and (int(m[1]), int(m[2])) == (original, handle)]
            release = [m for m in retired if begin <= m.start() < end and (int(m[1]), int(m[2])) == (original, handle)]
            assert len(mapping) == len(release) == 1
            mapping, release = mapping[0], release[0]
            original_gpu, work_gpu = int(mapping[3],16), int(mapping[4],16)
            assert original_gpu == p['gpu'] and 0x40000000 <= work_gpu < 0x80000000 and work_gpu % 16384 == 0
            assert work_gpu not in {value['gpu'] for value in expected}
            assert tuple(int(mapping[i]) for i in (5,6,7,8,9)) == (p['load'], p['width'], p['height'], cpp, cpp)
            assert page.end() < mapping.start() < release.start()
            copied = [m for m in copies if begin <= m.start() < end and (int(m[1]),int(m[2])) == (original,handle)]
            assert [m[3] for m in copied] == (['load'] if p['load'] == 1 else []) + (['store'] if p['publish'] else [])
            sequence = []
            for copy in copied:
                assert tuple(int(copy[i]) for i in (4,5,6)) == (p['samples'],p['width'],p['height'])
                assert mapping.end() < copy.start() < release.start() and int(copy[7]) > 0
                sequence.append(int(copy[7]))
                if revision==3:
                    candidates=[m for m in blits if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]),m[3])==(original,handle,copy[3])]
                    actual_boxes=[m for m in boxes if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]))==(original,handle)]
                    assert len(candidates)==1 and actual_boxes
                    blit=candidates[0];box=actual_boxes[-1]
                    assert mapping.end()<blit.start()<blit.end()<box.start()<box.end()<copy.start()
                    assert int(blit[4])==5 and blit[5]==blit[6] and blit[7]==blit[8] and int(blit[7])>0
                    assert tuple(int(blit[i]) for i in (9,10,11,12,13,14))==(1,1,15,15,15,0)
                    assert tuple(int(blit[i]) for i in (15,16))==((0,2) if copy[3]=='load' else (2,0))
                    expected_box=(0,0,0,p['width'],p['height'],1)
                    assert tuple(map(int,box[3].split(',')))==tuple(map(int,box[4].split(',')))==expected_box
                    assert tuple(map(int,box[5].split(',')))==tuple(map(int,box[6].split(',')))==(p['width'],p['height'],1)
                    wanted=(p['pitch'],pitch,p['offset'],0) if copy[3]=='load' else (pitch,p['pitch'],0,p['offset'])
                    assert tuple(int(box[i]) for i in (7,8,9,10))==wanted
            assert sequence == sorted(set(sequence)), 'Load/store must use actual ordered nonzero fences'
            name = 'DEPTH' if p['format'] == 252 else 'STENCIL'
            imports = list(re.finditer(r'SWEET METAL '+name+r' import handle='+str(original)+r' width='+str(p['width'])+r' height='+str(p['height'])+r' pitch='+str(p['pitch'])+r' offset='+str(p['offset'])+r' GPU='+hex(p['gpu'])+r' original_pages=1', segment[begin:end]))
            assert len(imports) == 1
            import_at = begin + imports[0].start()
            if p['load'] == 1: assert copied[0].end() < import_at
            if p['publish']:
                assert import_at < copied[-1].start()
                finishes = list(re.finditer(r'SWEET METAL '+name+r' retired handle='+str(original)+r' result=0 sequence=(\d+) original_pages=1',segment[begin:end]))
                assert len(finishes)==1 and int(finishes[0][1])==sequence[-1]
                assert copied[-1].end() < begin+finishes[0].start() < release.start()
            work[format_] = handle; work_gpus[format_] = work_gpu
            spans.append(dict(original=original, work=handle, work_GPU=hex(work_gpu), format=format_,
                              samples=samples, pitch=pitch, bytes=size, load=p['load'], publish=p['publish'], copy_sequences=sequence))
        assert len(set(work.values())) == len(group) and len(set(work_gpus.values())) == len(group)
        if 253 in work:
            paired = [m for m in pairs if begin <= m.start() < end]
            assert len(paired) == 1
            p = paired[0]; depth = next((v for v in group if v['format']==252),None); stencil = next(v for v in group if v['format']==253)
            assert tuple(map(int,p.groups())) == (depth['handle'] if depth else 0,stencil['handle'],work.get(252,0),work[253],stencil['samples'])
            assert max(m.end() for m in own_pages) < p.start() < min(m.start() for m in retired if begin<=m.start()<end)
        records.append(spans); index = following
    return dict(revision=revision, independent_FDL5_work_pages=True, GPU_copy_and_fence_verified=True,
                original_stride_offset_and_IOVA_verified=True, work_reference_retirement_verified=True,
                actual_designated_pipe_box_and_raw_copy_metadata_verified=revision==3,
                operations=len(groups), planes=len(expected), records=records)
