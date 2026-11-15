"""Expand original sampler buffer records and compare every byte and guard."""
import re
import struct
from air_render_evidence import fnv


def expected(phase):
    target=bytearray([0xa5]*16384)
    colour=[bytes((0,0,255,255)),bytes((0,255,255,255)),bytes((255,0,0,255))][phase]
    for y in range(19):
        target[64+y*192:64+y*192+132]=colour*33
    source=bytearray([0xb6]*16384)
    source[128:136]=bytes((255,0,0,255,0,255,0,255))
    source[192:200]=bytes((0,0,255,255,255,255,255,255))
    source[256:260]=bytes((0,0,255,255))
    vertices=bytearray([0xd4]*16384)
    vertices[64:112]=struct.pack('<12f',-1,1,.5,1,3,1,.5,1,-1,-3,.5,1)
    return target,source,vertices


def expand(block,phase,buffer):
    prefix=r'SWEET AIR SAMPLER (data|run) phase='+str(phase)+r' buffer='+str(buffer)
    rows=re.findall(prefix+r' offset=(\d+) bytes=(\d+) (?:hex|pattern)=([0-9a-f]+)',block)
    result=bytearray()
    for kind,offset,size,data in rows:
        offset=int(offset);size=int(size);raw=bytes.fromhex(data)
        assert offset==len(result) and size>0 and size<=16384-offset,'Sampler dump has a gap, overlap or out-of-bounds record'
        if kind=='run':
            assert len(raw) in (1,4) and size%len(raw)==0
            raw=raw*(size//len(raw))
        else:
            assert size<=64 and len(raw)==size
        result.extend(raw)
    assert len(result)==16384,'Sampler raw buffer incomplete'
    return result


def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET AIR SAMPLER END result=[1-9]',scope)
    starts=list(re.finditer(r'SWEET AIR SAMPLER begin phases=3 original_word=807bff0000080a49 literal_states=1 explicit_LOD=1 source_slot=5 external_samplers=0 original_pages=1',scope))
    assert starts
    complete=[];incomplete=0
    for i,start in enumerate(starts):
        block=scope[start.end():starts[i+1].start() if i+1<len(starts) else len(scope)]
        end=re.search(r'SWEET AIR SAMPLER END result=(\d+) checked_bytes=147456 phases=3 final_allocated=(\d+) GPU_only=1',block)
        if not end:incomplete+=1;continue
        assert end.groups()==('0','0');block=block[:end.start()]
        assert 'SWEET AIR SAMPLER input library_sha256='+fixture['library_sha256'] in block
        phases=re.findall(r'SWEET AIR SAMPLER phase=([012]) sequence=(\d+) operations=1 status=4 error=0 checked_bytes=49152 pixels=627 mismatches=0 original_pages=1 external_samplers=0 valid=1',block)
        if len(phases)!=3:incomplete+=1;continue
        assert [int(x[0]) for x in phases]==[0,1,2]
        sequences=[int(x[1]) for x in phases];assert sequences==sorted(set(sequences)) and sequences[0]>0
        hashes=[]
        for phase in range(3):
            buffers=expected(phase)
            for buffer,wanted in enumerate(buffers):
                raw=expand(block,phase,buffer);assert raw==wanted,'Original sampler result/source/vertex/guard bytes differ'
                h=fnv(raw);declared=re.findall(r'SWEET AIR SAMPLER buffer phase='+str(phase)+r' index='+str(buffer)+r' raw_hash=([0-9a-f]{16}) original_pages=1',block)
                assert declared==[h];hashes.append(h)
        complete.append({'sequences':sequences,'bytes_checked':147456,'target_pixels':1881,'original_buffer_hashes':hashes,
                         'original_literal_word_and_two_distinct_states_verified':True,'explicit_LOD_and_source_slot_5_verified':True,
                         'no_external_sampler_object_required':True,'every_original_byte_and_guard_verified':True,'final_allocated':0})
    assert complete,'No complete original sampler GPU sample'
    return {'complete_samples':len(complete),'incomplete_samples':incomplete,'samples':complete,
            'fixture_library_sha256':fixture['library_sha256'],'target_panel_UI_not_inferred':True}
