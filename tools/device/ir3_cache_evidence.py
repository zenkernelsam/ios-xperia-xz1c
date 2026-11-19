"""Original specialized kernels on fresh objects must restore hardware variants."""
import re,struct

def expected(phase):
    total=sum((13,29,13)[:phase+1]);h=14695981039346656037
    for byte in struct.pack('<80I',*(((((i*0x9e3779b1)&0xffffffff)^0x434f4e53)+(total if i<64 else 0))&0xffffffff for i in range(80))):h=((h^byte)*1099511628211)&0xffffffffffffffff
    return f'{h:016x}'

def verify(text,token,fixture):
    live='SWEET LOOP LIVE proto=1 token='+token;ack='SWEET LOOP ACK FASTBOOT token='+token
    assert text.count(live)==text.count(ack)==1;scope=text[text.index(live):text.index(ack)]
    begins=list(re.finditer(r'SWEET IR3 CACHE PROBE begin phases=3 steps=13,29,13 original_pages=1 fresh_pipelines=1',scope));assert len(begins)==1
    block=scope[begins[0].end():];end=re.search(r'SWEET IR3 CACHE PROBE END result=0 phases=3 checked_bytes=960 final_allocated=0 cold_and_restore=1 original_pages=1',block)
    assert end and not re.search(r'SWEET IR3 CACHE PROBE END result=[1-9]',scope);block=block[:end.start()]
    assert 'SWEET IR3 CACHE PROBE input library_sha256='+fixture['library_sha256'] in block
    rows=list(re.finditer(r'SWEET IR3 CACHE PROBE phase=([0-2]) step=(13|29) total=(13|42|55) sequence=(\d+) operations=1 checked_bytes=320 mismatches=0 hash=([0-9a-f]{16}) valid=1',block))
    stats=list(re.finditer(r'SWEET IR3 CACHE PROBE stats phase=([0-2]) hits=(\d+) stores=(\d+) rejected=0 bytes_read=(\d+) exact_variant=1',block))
    assert [(int(r[1]),int(r[2]),int(r[3])) for r in rows]==[(0,13,13),(1,29,42),(2,13,55)] and [int(r[1]) for r in stats]==[0,1,2]
    sequences=[int(r[4]) for r in rows];assert sequences==sorted(set(sequences)) and sequences[0]>0
    assert [r[5] for r in rows]==[expected(p) for p in range(3)]
    assert int(stats[2][2])>0 and int(stats[2][4])>0
    for phase in range(3):assert rows[phase].end()<stats[phase].start()
    return dict(complete_samples=1,incomplete_samples=0,samples=[dict(sequences=sequences,hits=[int(s[2]) for s in stats],
        stores=[int(s[3]) for s in stats],raw_hashes=[r[5] for r in rows],checked_bytes=960,final_allocated=0,
        original_specialization_and_guard_bytes_equal=True,actual_fresh_pipeline_hardware_variant_restore=True)],original_UI_FPS_not_inferred=True)
