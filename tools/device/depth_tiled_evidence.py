"""Bind actual FDL5 work planes, GPU copies and retirement to authored pages."""
import re

PAGE = re.compile(r'SWEET DEPTH TILE PAGE original=(\d+) work=(\d+) format=(\d+) samples=(\d+) pitch=(\d+) bytes=(\d+) tile=(\d+) original_pitch=(\d+) original_offset=(\d+)')
MAP = re.compile(r'SWEET DEPTH TILE MAP original=(\d+) work=(\d+) original_GPU=(0x[0-9a-f]+) work_GPU=(0x[0-9a-f]+) load=(\d+) width=(\d+) height=(\d+) cpp=(\d+) raw_cpp=(\d+) alias_samples=1')
COPY = re.compile(r'SWEET DEPTH TILE COPY original=(\d+) work=(\d+) direction=(load|store) samples=(\d+) width=(\d+) height=(\d+) result=0 sequence=(\d+) GPU_only=1 fence=1')
PAIR = re.compile(r'SWEET DEPTH TILE PAIR depth=(\d+) stencil=(\d+) depth_work=(\d+) stencil_work=(\d+) samples=(\d+) result=0 separate_z32s8=1')
RETIRE = re.compile(r'SWEET DEPTH TILE RETIRE original=(\d+) work=(\d+) result=0 references_retired=1')
BLIT = re.compile(r'SWEET DEPTH TILE BLIT original=(\d+) work=(\d+) direction=(load|store) gen=(\d+) src_fmt=(\d+) dst_fmt=(\d+) src_color=(\d+) dst_color=(\d+) src_samples=(\d+) dst_samples=(\d+) src_mask=(\d+) dst_mask=(\d+) mask=(\d+) filter=(\d+) src_tile=(\d+) dst_tile=(\d+)')
BOX = re.compile(r'SWEET DEPTH TILE BOX original=(\d+) work=(\d+) src=(-?\d+,-?\d+,-?\d+,-?\d+,-?\d+,-?\d+) dst=(-?\d+,-?\d+,-?\d+,-?\d+,-?\d+,-?\d+) src_size=(\d+,\d+,\d+) dst_size=(\d+,\d+,\d+) src_pitch=(\d+) dst_pitch=(\d+) src_offset=(\d+) dst_offset=(\d+)')
SAMPLE = re.compile(r'SWEET DEPTH TILE SAMPLE original=(\d+) work=(\d+) direction=(load|store) samples=(\d+) width=(\d+) height=(\d+) src_samples=(\d+) dst_samples=(\d+) passes=(\d+) masks=(\d+) bitcast=1 GPU_only=1')
STATE = re.compile(r'SWEET DEPTH TILE STATE original=(\d+) work=(\d+) direction=(load|store) all_dirty=(\d+) stencil_ref=(\d+) sample_mask=(\d+) scissor=(\d+) gen_dirty=(\d+) gen_all=(\d+)')
STENCIL_SAMPLE = re.compile(r'SWEET STENCIL TILE SAMPLE original=(\d+) work=(\d+) direction=(load|store) samples=(\d+) width=(\d+) height=(\d+) src_samples=(\d+) dst_samples=(\d+) passes=(\d+) masks=(\d+) bitplanes=8(?: zero_draw=([01]))? GPU_only=1')
DUMMY = re.compile(r'SWEET DEPTH TILE DUMMY stencil=(\d+) samples=(\d+) width=(\d+) height=(\d+) tile=(\d+) cpp=(\d+) pitch=(\d+) bytes=(\d+)(?: work=(\d+) work_GPU=(0x[0-9a-f]+))? depth_test=0 depth_writes=0 caller_depth=0')
DUMMY_RETIRE=re.compile(r'SWEET DEPTH TILE DUMMY_RETIRE work=(\d+) result=0 references_retired=1')


def plane(handle, gpu, format_, samples, pitch, load, publish=True,resolve=None):
    value=dict(handle=int(handle), gpu=int(gpu, 16) if isinstance(gpu, str) else gpu,
                format=format_, samples=samples, pitch=pitch, offset=64, width=17, height=9,
                load=load, publish=publish)
    if resolve is not None:value['resolve']=resolve
    return value


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
    assert revision in (2,3,4,5,6,7,8,9,10)
    tile_mode=3 if revision>=4 else 2
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
    sampled = [m for m in SAMPLE.finditer(segment) if int(m[1]) in handles]
    states = [m for m in STATE.finditer(segment) if int(m[1]) in handles]
    stencil_sampled = [m for m in STENCIL_SAMPLE.finditer(segment) if int(m[1]) in handles]
    dummies=[m for m in DUMMY.finditer(segment) if int(m[1]) in handles]
    dummy_releases=list(DUMMY_RETIRE.finditer(segment))
    if revision>=10:assert segment.count('SWEET DEPTH TILE DUMMY_RETIRE ')==len(dummy_releases)
    if revision>=9:
        assert segment.count('SWEET DEPTH TILE DUMMY ')==len(list(DUMMY.finditer(segment)))
        assert len(dummies)==sum(any(p['format']==253 for p in g) and not any(p['format']==252 for p in g) for g in groups)
    else:assert not dummies
    if revision>=3:
        assert 'SWEET DEPTH TILE REJECT ' not in segment
        assert segment.count('SWEET DEPTH TILE BLIT ')==len(list(BLIT.finditer(segment)))
        assert segment.count('SWEET DEPTH TILE BOX ')==len(list(BOX.finditer(segment)))
        assert len(blits)==len(boxes) and len(blits)+len(sampled)+len(stencil_sampled)==len(copies), 'Original GPU copy metadata missing'
        assert segment.count('SWEET DEPTH TILE SAMPLE ')==len(list(SAMPLE.finditer(segment)))
        if revision<5:assert not sampled
        if revision<7:assert not stencil_sampled
        else:assert segment.count('SWEET STENCIL TILE SAMPLE ')==len(list(STENCIL_SAMPLE.finditer(segment)))
        if revision>=6:
            assert segment.count('SWEET DEPTH TILE STATE ')==len(list(STATE.finditer(segment)))
            assert len(states)==len(blits), 'Native 3D state replay missing after an internal 2D batch'
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
            assert handle > 0 and handle not in handles and (pitch, size, tile) == (expected_pitch, expected_size, tile_mode)
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
                if revision>=7 and p['format']==253:
                    candidates=[m for m in stencil_sampled if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]),m[3])==(original,handle,copy[3])]
                    assert len(candidates)==1
                    sample=candidates[0];count=p['samples'];wanted=(count,p['width'],p['height'],1,count,1+8*count,(1<<count)-1) if copy[3]=='load' else (count,p['width'],p['height'],count,1,1,0)
                    assert tuple(int(sample[i]) for i in range(4,11))==wanted
                    if revision>=8:assert sample[11]==('1' if copy[3]=='load' else '0'), 'Actual GPU stencil reset draw missing'
                    else:assert sample[11] is None
                    assert mapping.end()<sample.start()<sample.end()<copy.start()
                elif revision>=5 and p['format']==252 and p['samples']>1:
                    candidates=[m for m in sampled if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]),m[3])==(original,handle,copy[3])]
                    assert len(candidates)==1
                    sample=candidates[0];count=p['samples'];wanted=(count,p['width'],p['height'],1,count,count,(1<<count)-1) if copy[3]=='load' else (count,p['width'],p['height'],count,1,1,0)
                    assert tuple(int(sample[i]) for i in range(4,11))==wanted
                    assert mapping.end()<sample.start()<sample.end()<copy.start()
                elif revision>=3:
                    candidates=[m for m in blits if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]),m[3])==(original,handle,copy[3])]
                    actual_boxes=[m for m in boxes if begin<=m.start()<copy.start() and (int(m[1]),int(m[2]))==(original,handle)]
                    assert len(candidates)==1 and actual_boxes
                    blit=candidates[0];box=actual_boxes[-1]
                    assert mapping.end()<blit.start()<blit.end()<box.start()<box.end()<copy.start()
                    assert int(blit[4])==5 and blit[5]==blit[6] and blit[7]==blit[8] and int(blit[7])>0
                    assert tuple(int(blit[i]) for i in (9,10,11,12,13,14))==(1,1,15,15,15,0)
                    assert tuple(int(blit[i]) for i in (15,16))==((0,tile_mode) if copy[3]=='load' else (tile_mode,0))
                    expected_box=(0,0,0,p['width'],p['height'],1)
                    assert tuple(map(int,box[3].split(',')))==tuple(map(int,box[4].split(',')))==expected_box
                    assert tuple(map(int,box[5].split(',')))==tuple(map(int,box[6].split(',')))==(p['width'],p['height'],1)
                    wanted=(p['pitch'],pitch,p['offset'],0) if copy[3]=='load' else (pitch,p['pitch'],0,p['offset'])
                    assert tuple(int(box[i]) for i in (7,8,9,10))==wanted
                    if revision>=6:
                        replay=[m for m in states if copy.end()<m.start()<release.start() and (int(m[1]),int(m[2]),m[3])==(original,handle,copy[3])]
                        assert len(replay)==1
                        replay=replay[0];assert tuple(int(replay[i]) for i in (4,5,6,7))==(1,1,1,1)
                        # FD5 uses the common FD_DIRTY_* state mask; unlike
                        # FD6+, its pinned context installs no gen_dirty maps.
                        # The real zero/zero gen masks are valid only alongside
                        # the actual all_dirty/stencil/sample/scissor flags.
                        assert int(replay[8])==int(replay[9])
                        assert copy.end()<replay.start()<replay.end()<release.start()
            assert sequence == sorted(set(sequence)), 'Load/store must use actual ordered nonzero fences'
            name = 'DEPTH' if p['format'] == 252 else 'STENCIL'
            imports = list(re.finditer(r'SWEET METAL '+name+r' import handle='+str(original)+r' width='+str(p['width'])+r' height='+str(p['height'])+r' pitch='+str(p['pitch'])+r' offset='+str(p['offset'])+r' GPU='+hex(p['gpu'])+r' original_pages=1', segment[begin:end]))
            assert len(imports) == 1
            import_at = begin + imports[0].start()
            if p['load'] == 1: assert copied[0].end() < import_at
            if p['publish']:
                assert import_at < copied[-1].start()
                finishes = list(re.finditer(r'SWEET METAL '+name+r' retired handle='+str(original)+r' result=0 sequence=(\d+) original_pages=1',segment[begin:end]))
                if p.get('resolve') is not None:
                    src,count,filter_=p['resolve'];assert p['format']==252 and p['samples']==1 and p['load']==0
                    assert not finishes
                    pattern=r'SWEET MSAA resolve src='+str(src)+r' dst='+str(original)+r' samples='+str(count)+r' aspect=1 filter='+str(filter_)+r' result=0 sequence=(\d+) original_pages=1 GPU_only=1'
                    resolved=list(re.finditer(pattern,segment[begin:end]));assert len(resolved)==1 and int(resolved[0][1])==sequence[-1]
                    assert copied[-1].end()<release.start()<begin+resolved[0].start()<end
                else:
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
            if revision>=9 and depth is None:
                dummy=[m for m in dummies if begin<m.start()<m.end()<p.start()];assert len(dummy)==1
                _,dummy_pitch,dummy_size=layout_bytes(252,stencil['samples'],stencil['width'],stencil['height'])
                dummy=dummy[0]
                assert tuple(int(dummy[i]) for i in range(1,9))==(stencil['handle'],stencil['samples'],stencil['width'],stencil['height'],3,4*stencil['samples'],dummy_pitch,dummy_size)
                if revision>=10:
                    assert dummy[9] is not None and dummy[10] is not None
                    dwork,dgpu=int(dummy[9]),int(dummy[10],16)
                    assert dwork>0 and dwork not in handles|set(work.values())
                    assert 0x40000000<=dgpu<0x80000000 and dgpu%16384==0 and dgpu not in set(work_gpus.values())|{x['gpu'] for x in expected}
                    d_releases=[m for m in dummy_releases if begin<m.start()<m.end()<end and int(m[1])==dwork];assert len(d_releases)==1
                    assert dummy.end()<d_releases[0].start()<min(m.start() for m in retired if begin<m.start()<end)
                    if copied:assert max(m.end() for m in copied)<d_releases[0].start()
                else:assert dummy[9] is dummy[10] is None
        records.append(spans); index = following
    return dict(revision=revision, independent_FDL5_work_pages=True, GPU_copy_and_fence_verified=True,
                original_stride_offset_and_IOVA_verified=True, work_reference_retirement_verified=True,
                actual_designated_pipe_box_and_raw_copy_metadata_verified=revision>=3 and bool(blits),
                actual_pinned_fd5_sysmem_tile_mode_verified=revision>=4,
                actual_depth_MSAA_sample_shader_bitcast_verified=revision>=5 and bool(sampled),
                native_3D_state_marked_dirty_after_internal_2D_copy=revision>=6 and bool(states),
                actual_stencil_texture_GPU_bitplane_transfer_verified=revision>=7 and bool(stencil_sampled),
                stencil_only_unused_depth_primary_native_tile_verified=revision>=9 and bool(dummies),
                operations=len(groups), planes=len(expected), records=records)
