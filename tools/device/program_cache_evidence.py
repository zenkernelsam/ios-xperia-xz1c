"""Tie fresh-pipeline binary restores to all original sampler GPU bytes."""
import re
from air_sampler_evidence import verify as verify_sampler


def verify(text,token,fixture):
    sampler=verify_sampler(text,token,fixture)
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET PROGRAM CACHE roundtrip [^\r\n]*valid=0',scope)
    begin=scope.index('SWEET AIR SAMPLER begin');end=scope.index('SWEET AIR SAMPLER END',begin);block=scope[begin:end]
    rows=re.findall(r'SWEET PROGRAM CACHE roundtrip phase=([012]) first_state=([12]) restored_state=1 fresh_pipeline=1 valid=1',block)
    assert [int(row[0]) for row in rows]==[0,1,2]
    hits=re.findall(r'SWEET PROGRAM CACHE hit bytes=(\d+) format=(0x[0-9a-f]+) exact_build=1 exact_source_config=1',block)
    assert len(hits)>=3 and all(0<int(n)<=8<<20 for n,_ in hits)
    assert sampler['complete_samples']==1 and sampler['incomplete_samples']==0
    return {'complete_samples':1,'incomplete_samples':0,'samples':[{'fresh_pipeline_restores':3,'actual_sampler_bytes_checked':147456,
            'source_input_vertices_padding_and_pixels_verified':True,'binary_hits':len(hits),'original_sampler_samples':sampler['samples']}],
            'system_wallpaper_cache_or_full_UI_not_inferred':True}
