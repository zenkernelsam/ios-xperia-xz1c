"""Check every original binary16 static-memory result and all target guard bytes."""
import re
import struct
from air_render_evidence import fnv

PATTERNS=(0x0000,0x8000,0x0001,0x03ff,0x0400,0x3c00,0x7bff,0x7c00,0xfc00,0x7e55,0x7d01,0xfe01)


def verify(text,token,fixture):
    scope=text[text.index('SWEET LOOP LIVE proto=1 token='+token):text.index('SWEET LOOP ACK FASTBOOT token='+token)]
    assert not re.search(r'SWEET AIR STATIC HALF END result=[1-9]|SWEET AIR STATIC HALF case=[^\r\n]*valid=0',scope)
    starts=list(re.finditer(r'SWEET AIR STATIC HALF begin cases=12 paths=4 original_constants=1 original_pages=1 GPU_only=1',scope))
    assert len(starts)==1
    block=scope[starts[0].end():]
    end=re.search(r'SWEET AIR STATIC HALF END result=0 cases=12 paths=4 checked_bytes=196608 final_allocated=0 original_constants=1 GPU_only=1',block)
    assert end,'Static half probe incomplete';block=block[:end.start()]
    assert 'SWEET AIR STATIC HALF input library_sha256='+fixture['library_sha256'] in block
    rows=re.findall(r'SWEET AIR STATIC HALF case=(\d+) half=([0-9a-f]{4}) word=([0-9a-f]{4}) status=4 error=0 sequence=(\d+) operations=1 checked_bytes=16384 mismatches=0 hash=([0-9a-f]{16}) frozen_after_reset=1 valid=1',block)
    assert [int(row[0]) for row in rows]==list(range(12))
    sequences=[int(row[3]) for row in rows];assert sequences==sorted(set(sequences)) and sequences[0]>0
    hashes=[]
    for case,half,word,sequence,declared in rows:
        n=int(case);h=int(half,16);w=int(word,16);assert (h,w)==(PATTERNS[n],PATTERNS[(n+1)%12])
        actual=bytearray()
        records=re.findall(r'SWEET AIR STATIC HALF (run|data) case='+case+r' offset=(\d+) bytes=(\d+) (?:pattern|hex)=([0-9a-f]+)',block)
        assert len(records)==3
        for kind,offset,count,value in records:
            offset=int(offset);count=int(count);data=bytes.fromhex(value)
            assert offset==len(actual) and count>0 and count<=16384-offset
            if kind=='run':assert data==b'\xa5';data*=count
            else:assert count==16 and len(data)==count
            actual.extend(data)
        assert len(actual)==16384
        expected=bytearray([0xa5]*16384);struct.pack_into('<4I',expected,64,h,h,w,w)
        assert actual==expected,'Original static half output or guard bytes differ'
        assert fnv(actual)==declared;hashes.append(declared)
    return {'complete_samples':1,'incomplete_samples':0,'samples':[{'sequences':sequences,'bytes_checked':196608,
        'raw_hashes':hashes,'half_and_integer_load_store_paths':4,'specialization_snapshot_preserved':True,
        'signed_zero_subnormals_infinities_and_NaN_payloads_checked':True,'every_original_byte_and_guard_verified':True,'final_allocated':0}],
        'fixture_library_sha256':fixture['library_sha256'],'actual_UI_not_inferred':True}
