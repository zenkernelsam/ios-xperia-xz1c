"""Recompute authored original MSAA pages, resolves, guards and queue isolation."""
import re
import struct

BEGIN='SWEET MULTISAMPLE BEGIN phases=18 width=17 height=9 samples=2,4 source_samples=original resolve=GPU_shader'
END='SWEET MULTISAMPLE END result=0 phases=18 checked_bytes=151488 guard_bytes=55098 final_allocated=0 GPU_only=1 original_samples=1'
SLOTS=(0,8,9,10,11)
PAGE=re.compile(r'SWEET MULTISAMPLE PAGE phase=(\d+) slot=(\d+) public=([0-9a-f]{16}) pass=(\d+) handle=(\d+) GPU=(0x[0-9a-f]+) bytes=(\d+) pitch=(\d+) offset=(\d+) format=(\d+) samples=(\d+)')
PHASE=re.compile(r'SWEET MULTISAMPLE PHASE index=(\d+) valid=([01]) sequence=(\d+) operations=(\d+) checked_bytes=(\d+) guard_bytes=(\d+) mismatches=(\d+) original_pages=1')
SOURCE=re.compile(r'SWEET MULTISAMPLE SOURCE index=(\d+) color=([0-9a-f]{16}) depth=([0-9a-f]{16}) stencil=([0-9a-f]{16})')
RESOLVED=re.compile(r'SWEET MULTISAMPLE RESOLVED index=(\d+) color=([0-9a-f]{16}) depth=([0-9a-f]{16})')
ISOLATION=re.compile(r'SWEET MULTISAMPLE ISOLATION samples=([24]) valid=1 two_queues=1 original_public_shared=1 private_pages_differ=1 encoders_before_commit=1')

def samples(phase):
    assert 0<=phase<18
    return 2 if phase<7 or phase in (14,15) else 4

def state(phase):
    if phase>=14:return 1 if phase%2==0 else 2
    return (0,1,2,2,2,2,3)[phase%7]

def pitch(count,slot):
    return 128 if slot>=10 else (64 if count==2 else 128) if slot==9 else (192 if count==2 else 320)

def size(count,slot):return 128+pitch(count,slot)*9

def operations(phase):
    if phase>=14:return samples(phase)+2+phase%2
    return (1,samples(phase),1,2,2,2,3)[phase%7]

def draw_masks(phase):
    if phase>=14:return [1<<i for i in range(samples(phase))]+([1] if phase%2 else [])
    return [1<<i for i in range(samples(phase))] if phase%7==1 else [1] if phase%7==2 else [0xffffffff] if phase%7==6 else []

def filter_mode(phase):return 1 if phase<14 and phase%7==4 else 2 if phase<14 and phase%7==5 else 0

def rgba(sample,version):
    return (255,0,255,255) if version==0 else (64,128,191,255) if version==3 else (0,224,224,255) if version==2 and sample==0 else (
        (32,64,96,255),(96,128,160,255),(160,192,224,255),(224,64,32,255))[sample]

def depth(sample,version):return 0.75 if version==0 else 0.25 if version==3 else 0.375 if version==2 and sample==0 else (0.5,0.125,0.75,0.25)[sample]

def stencil(sample,version):return 17 if version==0 else 77 if version==3 else 90 if version==2 and sample==0 else 40+sample

def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def expected_bytes(phase,slot):
    assert slot in SLOTS
    count=samples(phase);row=pitch(count,slot);data=bytearray([{0:0xa5,8:0xb6,9:0xc7,10:0xd8,11:0xe9}[slot]]*size(count,slot))
    resolved=phase>=14 or phase%7>=3
    if slot>=10 and not resolved:return bytes(data)
    for y in range(9):
        for x in range(17):
            version=state(phase) if 3<=x<14 and 2<=y<7 else 0
            if slot<10:
                for sample in range(count):
                    at=64+y*row+x*(1 if slot==9 else 4)*count+sample*(1 if slot==9 else 4)
                    if slot==0:
                        r,g,b,a=rgba(sample,version);data[at:at+4]=bytes((b,g,r,a))
                    elif slot==8:data[at:at+4]=struct.pack('<f',depth(sample,version))
                    else:data[at]=stencil(sample,version)
            else:
                at=64+y*row+x*4
                if slot==10:
                    colors=[rgba(i,version) for i in range(count)];r,g,b,a=[sum(c[i] for c in colors)//count for i in range(4)];data[at:at+4]=bytes((b,g,r,a))
                else:
                    values=[depth(i,version) for i in range(count)];mode=filter_mode(phase)
                    data[at:at+4]=struct.pack('<f',min(values) if mode==1 else max(values) if mode==2 else values[0])
    return bytes(data)

def expected(phase):return tuple(fnv(expected_bytes(phase,slot)) for slot in SLOTS)

def verify(text,token,fixture,tiled_revision=0):
    assert tiled_revision in (0,2,3,4,5,6,7,8)
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1
    scope=text[text.index(live):text.index(ack)]
    assert scope.count(BEGIN)==scope.count('SWEET MULTISAMPLE BEGIN ')==1 and scope.count(END)==scope.count('SWEET MULTISAMPLE END ')==1
    assert scope.index(BEGIN)<scope.index(END)
    block=scope[scope.index(BEGIN):scope.index(END)]
    assert block.count('SWEET MULTISAMPLE INPUT fixture_sha256='+fixture['library_sha256'])==1
    pages=list(PAGE.finditer(block));phases=list(PHASE.finditer(block));sources=list(SOURCE.finditer(block));resolved=list(RESOLVED.finditer(block));isolation=list(ISOLATION.finditer(block))
    assert len(pages)==90 and len(phases)==len(sources)==len(resolved)==18 and len(isolation)==2
    assert block.count('SWEET MULTISAMPLE ')==1+1+90+18*3+2,'Unknown, duplicated, truncated or failed MSAA record'
    assert [int(p[1]) for p in phases]==[int(p[1]) for p in sources]==[int(p[1]) for p in resolved]==list(range(18))
    original={};records=[];sequences=[];total=guards=0;tiled=[]
    for phase,(p,s,r) in enumerate(zip(phases,sources,resolved)):
        count=samples(phase);checked=sum(size(count,slot) for slot in SLOTS);guard=checked-17*9*(9*count+8)
        sequence=int(p[3]);assert sequence>0 and sequence not in sequences and p[2]=='1' and p[7]=='0'
        assert (int(p[4]),int(p[5]),int(p[6]))==(operations(phase),checked,guard)
        assert tuple(s.group(i) for i in (2,3,4))+tuple(r.group(i) for i in (2,3))==expected(phase),'Original MSAA source, destination or guard bytes differ'
        assert p.end()<s.start()<r.start() and (phase==17 or r.end()<phases[phase+1].start())
        entries=[m for m in pages if int(m[1])==phase];assert tuple(int(m[2]) for m in entries)==SLOTS
        for m in entries:
            slot=int(m[2]);handle=int(m[5]);gpu=int(m[6],16);identity=int(m[3],16);pass_id=int(m[4])
            assert handle>0 and identity>0 and 0x40000000<=gpu<0x80000000 and gpu%16384==0
            assert tuple(int(m[i]) for i in range(7,12))==(size(count,slot),pitch(count,slot),64,80 if slot in (0,10) else 253 if slot==9 else 252,count if slot<10 else 1)
            assert bool(pass_id)==(phase>=14 and slot<10)
            assert m.end()<p.start()
            original[phase,slot]=dict(public=identity,pass_id=pass_id,handle=handle,gpu_address=m[6])
        assert len({original[phase,slot]['handle'] for slot in SLOTS})==len({original[phase,slot]['gpu_address'] for slot in SLOTS})==len({original[phase,slot]['public'] for slot in SLOTS})==5
        # All real backend draws/resolves precede the checked phase and are
        # bound to these retained pages, not a process-wide generic PASS.
        start=min(m.start() for m in entries);segment=block[start:p.start()]
        handles=[original[phase,slot]['handle'] for slot in SLOTS];c,z,st,rc,rz=handles
        draws=list(re.finditer(r'SWEET MSAA draw samples='+str(count)+r' mask=(\d+) color='+str(c)+r' depth='+str(z)+r' stencil='+str(st)+r' result=0 sequence=(\d+) original_pages=1',segment))
        assert [int(m[1]) for m in draws]==draw_masks(phase)
        assert all(0<int(m[2])<=sequence for m in draws)
        clears=list(re.finditer(r'SWEET MSAA clear samples='+str(count)+r' color='+str(c)+r' depth='+str(z)+r' stencil='+str(st)+r' result=0 sequence=(\d+) original_pages=1',segment))
        assert len(clears)==(1 if phase<14 and phase%7==0 else 0)
        assert all(0<int(m[1])<=sequence for m in clears)
        resolves=list(re.finditer(r'SWEET MSAA resolve src=(\d+) dst=(\d+) samples='+str(count)+r' aspect=(\d+) filter=(\d+) result=0 sequence=(\d+) original_pages=1 GPU_only=1',segment))
        resolves=[m for m in resolves if int(m[1]) in (c,z)]
        should=phase>=14 or phase%7>=3
        assert [(int(m[1]),int(m[2]),int(m[3]),int(m[4])) for m in resolves]==([(c,rc,0,0),(z,rz,1,filter_mode(phase))] if should else [])
        assert all(0<int(m[5])<=sequence for m in resolves)
        completed=[int(m[2]) for m in draws]+[int(m[1]) for m in clears]+[int(m[5]) for m in resolves]
        assert len(completed)==operations(phase) and len(set(completed))==len(completed) and max(completed)==sequence
        if draws and resolves:assert draws[-1].end()<resolves[0].start()
        for slot,name in ((0,'MRT'),(8,'DEPTH'),(9,'STENCIL')):
            handle=original[phase,slot]['handle']
            pattern=(r'SWEET METAL MRT retired index=0 handle='+str(handle)+r' format=80 result=0 sequence=(\d+) original_pages=1') if slot==0 else (
                r'SWEET METAL '+name+r' retired handle='+str(handle)+r' result=0 sequence=(\d+) original_pages=1')
            retired=list(re.finditer(pattern,segment));assert len(retired)==len(draws)+len(clears), f'Original retirement count differs phase={phase} slot={slot}'
            assert all(0<int(m[1])<=sequence for m in retired)
        # The importer must expose original real sample count, stride, offset
        # and the page's actual IOVA. S8 must be paired with the original Z32.
        for slot in (0,8,9):
            info=original[phase,slot];h=info['handle'];row=pitch(count,slot);format_=80 if slot==0 else 252 if slot==8 else 253
            suffix=r' result=0 separate_z32s8=1 original_pages=1' if slot==9 else r' result=0 original_pages=1'
            imported=list(re.finditer(r'SWEET MSAA '+('stencil_import' if slot==9 else 'import')+r' handle='+str(h)+r' width=17 height=9 pitch='+str(row)+r' offset=64 '+('samples='+str(count)+r' GPU=' if slot==9 else 'format='+str(format_)+r' samples='+str(count)+r' GPU=')+r'(0x[0-9a-f]+)'+suffix,segment))
            need=len(draws)+len(clears)+(1 if should and slot in (0,8) else 0)
            # Combined Z/S import does not produce a standalone depth record.
            if slot==8:need=1 if should else 0
            if tiled_revision and slot==9:need=0
            assert len(imported)==need and all(m[1]==info['gpu_address'] for m in imported)
        paired=list(re.finditer(r'SWEET MSAA pair depth='+str(z)+r' stencil='+str(st)+r' samples='+str(count)+r' result=0 separate_z32s8=1 original_pages=1',segment))
        assert len(paired)==(0 if tiled_revision else len(draws)+len(clears))
        if tiled_revision:
            from depth_tiled_evidence import plane,verify_groups
            groups=[[plane(original[phase,slot]['handle'],original[phase,slot]['gpu_address'],
                           253 if slot==9 else 252,count,pitch(count,slot),2 if clears or phase>=14 and operation==0 else 1)
                     for slot in (8,9)] for operation in range(len(draws)+len(clears))]
            if should:groups.append([plane(rz,original[phase,11]['gpu_address'],252,1,pitch(count,11),0)])
            tiled.append(dict(phase=phase,**verify_groups(segment,groups,tiled_revision)))
        if phase<14 and phase%7>0:
            for slot in SLOTS:assert original[phase,slot]==original[phase-1,slot],'A stored pass silently replaced its original source or resolve destination'
        if phase>=14:
            assert len({original[phase,slot]['pass_id'] for slot in (0,8,9)})==1
        sequences.append(sequence);total+=checked;guards+=guard;records.append(dict(phase=phase,samples=count,sequence=sequence,operations=operations(phase),checked_bytes=checked,hashes=expected(phase)))
    assert sequences[:14]==sorted(sequences[:14])
    assert all(value>sequences[13] for value in sequences[14:]) and min(sequences[16:])>max(sequences[14:16])
    assert [int(m[1]) for m in isolation]==[2,4]
    for start in (14,16):
        marker=isolation[(start-14)//2]
        page_end=max(m.end() for m in pages if int(m[1]) in (start,start+1))
        assert page_end<marker.start() and marker.end()<phases[start].start()
        for slot in (0,8,9):
            a=original[start,slot];b=original[start+1,slot]
            assert a['public']==b['public'] and a['handle']!=b['handle'] and a['gpu_address']!=b['gpu_address'] and a['pass_id']!=b['pass_id']
    assert (total,guards)==(151488,55098)
    result=dict(complete_samples=1,incomplete_samples=0,phases=18,checked_bytes=total,guard_bytes=guards,records=records,original_pages=[dict(phase=phase,slot=slot,**info) for (phase,slot),info in original.items()],
        original_sample_mask_and_Load_Clear_retention_verified=True,colour_average_depth_sample0_min_max_GPU_resolve_verified=True,source_samples_and_all_guards_preserved=True,
        Memoryless_two_queue_original_pages_are_independent=True,final_allocated=0,raw_is_actual_device_evidence_only_when_called_from_verified_trial=True)
    if tiled_revision:result['tiled_work_planes']=tiled
    return result
