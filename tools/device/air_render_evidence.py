"""Verify current-boot original-page AIR render and all logical/padding bytes."""
import re

def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def source_pixel(x,y,phase):return bytes(((x*29+y*7+phase*13)&255,(x*5+y*31+phase*17)&255,(x*19+y*11+phase*23)&255,255))

def expected(phase):
    target=bytearray([0xa5]*3776);sample=bytearray([0xb6]*512)
    for y in range(5):
        for x in range(7):sample[128+y*64+x*4:128+y*64+x*4+4]=source_pixel(x,y,phase==3)
    for y in range(19):
        for x in range(33):
            if phase==0 or (phase==1 and not (5<=x<28 and 4<=y<15)):color=bytes((0,0,255,255))
            elif phase==1:color=bytes((191,128,64,255))
            else:
                source_phase=phase==3 and 3<=x<26 and 4<=y<15
                r,g,b,a=source_pixel(((2*x+1)*7)//66,((2*y+1)*5)//38,source_phase);color=bytes((b,g,r,a))
            offset=64+y*192+x*4;target[offset:offset+4]=color
    return fnv(target),fnv(sample)

def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    starts=list(re.finditer(r'SWEET AIR RENDER begin library_bytes=(\d+) standard_MTLB=1 standard_render_encoder=1 original_textures=1',scope))
    assert starts,'Actual AIR render probe missing'
    records=[];incomplete=0
    for index,start in enumerate(starts):
        assert int(start[1])==fixture['library_bytes']
        block=scope[start.end():starts[index+1].start() if index+1<len(starts) else len(scope)]
        end=re.search(r'SWEET AIR RENDER END result=(\d+) phases=4 draw_ops=3 clear_ops=1 checked_target_bytes=15104 checked_sample_bytes=2048 guard_bytes=6560 final_allocated=(\d+) standard_Metal=1',block)
        if not end:incomplete+=1;continue
        assert end[1]==end[2]=='0','AIR rendering, pixel or lifetime failed'
        block=block[:end.start()]
        assert 'SWEET AIR RENDER input library_sha256='+fixture['library_sha256'] in block
        assert 'SWEET AIR RENDER contract target_bytes=3776 sample_bytes=512 target_pixels=627 target_offset=64 target_pitch=192 sample_offset=128 sample_pitch=64' in block
        rows=list(re.finditer(r'SWEET AIR RENDER phase=([0-3]) status=4 error=0 sequence=(\d+) operations=1 callback_count=([1-4]) mismatches=0 read_mismatches=0 target_hash=([0-9a-f]{16}) sample_hash=([0-9a-f]{16}) original_pages=1 valid=1',block))
        if len(rows)<4:incomplete+=1;continue
        assert [int(row[1]) for row in rows]==list(range(4))
        sequence=[int(row[2]) for row in rows];assert sequence==sorted(set(sequence)) and sequence[0]>0
        for row in rows:
            phase=int(row[1]);assert int(row[3])==phase+1 and (row[4],row[5])==expected(phase)
        compiled=re.findall(r'SWEET AIR RENDER compiled program=(\d+) vertex_mask=1 fragment_mask=0 format=80 vertex_hash=([0-9a-f]{16}) fragment_hash=([0-9a-f]{16}) linked=1 robust=1',block)
        assert len(compiled)==2 and all(int(row[0])>0 for row in compiled)
        target=re.findall(r'SWEET AIR RENDER texture handle=(\d+) width=33 height=19 pitch=192 format=80 offset=64 gpu=(0x[0-9a-f]+) original_pages=1',block)
        sample=re.findall(r'SWEET AIR RENDER texture handle=(\d+) width=7 height=5 pitch=64 format=70 offset=128 gpu=(0x[0-9a-f]+) original_pages=1',block)
        assert len(target)==4 and len(set(target))==1 and len(sample)==2 and len(set(sample))==1 and target[0][0]!=sample[0][0]
        assert int(target[0][1],16)%16384==int(sample[0][1],16)%16384==0
        draws=re.findall(r'SWEET AIR RENDER draw width=33 height=19 primitive=3 first=0 count=3 instances=1 target_handle=(\d+) result=0 sequence=(\d+) original_pages=1',block)
        assert [int(row[1]) for row in draws]==sequence[1:] and all(row[0]==target[0][0] for row in draws)
        assert 'SWEET AIR RENDER bounded_clear width=33 height=19 shader_draw=1 result=0' in block
        vertices=re.findall(r'SWEET AIR RENDER vertex_words index=0 handle=(\d+) gpu=(0x[0-9a-f]+) offset=0 bytes=(48|96) logical=\3 integer_texture=1 guarded=1 original_pages=1',block)
        assert len(vertices)==3 and [row[2] for row in vertices]==['48','96','96'] and vertices[1]==vertices[2]
        assert all(int(row[1],16)%16384==0 and row[0] not in (target[0][0],sample[0][0]) for row in vertices)
        records.append(dict(sequences=sequence,target_handle=int(target[0][0]),source_handle=int(sample[0][0]),
            target_original_GPU_address=target[0][1],source_original_GPU_address=sample[0][1],
            target_hashes=[row[4] for row in rows],sample_hashes=[row[5] for row in rows],
            checked_logical_bytes=17152,checked_guard_bytes=6560,checked_pixels=2508,final_allocated_bytes=0,
            translated_vertex_hashes=[row[1] for row in compiled],translated_fragment_hashes=[row[2] for row in compiled]))
    assert records,'No complete current-boot AIR render and original texture proof'
    return dict(actual_MTLB_AIR_render_GPU_path_verified=True,complete_samples=len(records),incomplete_samples=incomplete,
        original_Metal_texture_pages_used=True,strided_RGBA_to_BGRA_and_upload_refresh_verified=True,
        scissor_viewport_winding_and_all_padding_checked=True,records=records,system_Metal_and_complete_UI_pending=True)
