"""Bind real v726 memory-entry failures to current-token Surface creation and return."""
import argparse
import json
import re
from pathlib import Path
from trial_evidence import verify


def inspect(directory):
    root=Path(directory);proof=verify(root,'gpu',version=726)
    raw=(root/'cycle-01/serial.raw.log').read_text(errors='replace')
    token=proof['token'];start=raw.index('SWEET LOOP LIVE proto=1 token='+token)
    end=raw.index('SWEET LOOP ACK FASTBOOT token='+token)
    text=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',raw[start:end])
    records=[]
    pattern=r'SWEET SURF MEMREF ERROR address=(0x[0-9a-f]+) requested=(\d+) actual=(\d+) prot=(0x[0-9a-f]+) page_mask=(0x[0-9a-f]+) kr=(-?\d+) count=(\d+)'
    for match in re.finditer(pattern,text):
        address,requested,actual,prot,mask,kr,count=match.groups()
        count=int(count);assert 1<=count<=24
        records.append(dict(address=address,requested_bytes=int(requested),returned_size=int(actual),
            requested_protection=int(prot,16),map_page_bytes=int(mask,16)+1,kern_return=int(kr),counter=count))
    assert records,'No actual memory-entry failure captured'
    counts=[row['counter'] for row in records]
    assert len(set(counts))==len(counts)
    initialized={int(n):dict(length=int(size),flags=int(flags,16),kr=int(kr,16)) for size,flags,kr,n in
        re.findall(r'SWEET SURF MEMREF INIT length=(\d+) flags=(0x[0-9a-f]+) kr=(0x[0-9a-f]+) count=(\d+)',text)}
    for row in records:
        init=initialized[row['counter']]
        assert init['length']==row['requested_bytes'] and init['kr']==row['kern_return']
        row['descriptor_flags']=init['flags']
    properties=list(re.finditer(r'sweetgl: surface props via=init id=0 width=(\d+) height=(\d+) bpe=(\d+) pitch=(\d+) alloc=(\d+) address=(0x[0-9a-f]+) client_address=(0x[0-9a-f]+) offset=(\d+) keys=(\d+) returned=0x0 forwarded=1',text))
    requests=[]
    for row in records:
        base=int(row['address'],16);size=row['requested_bytes']
        candidates=[m for m in properties if base<=int(m[6],16)<base+size]
        if not candidates:continue
        m=candidates[0];assert m[6]==m[7]
        shape=dict(width=int(m[1]),height=int(m[2]),bpe=int(m[3]),pitch=int(m[4]),
                   raw_bytes=int(m[5]),source=m[6],offset_in_cpu_page=int(m[6],16)-base)
        assert (shape['offset_in_cpu_page']+shape['raw_bytes']+row['map_page_bytes']-1)//row['map_page_bytes']*row['map_page_bytes']==size
        requests.append(dict(counter=row['counter'],**shape))
    crashes=re.findall(r'CRASH AegirPoster\[(\d+)\] sig 6',text)
    return dict(version=726,token=token,kernel_sha256=proof['kernel_sha256'],
        image_sha256=proof['image_sha256'],log_sha256=proof['log_sha256'],
        strict_return=proof['return_complete'],memory_entry_records=records,matched_source_shapes=requests,
        memory_entry_record_count=len(records),
        all_captured_errors_are_KERN_PROTECTION_FAILURE=all(r['kern_return']==2 for r in records),
        all_captured_maps_are_16KiB=all(r['map_page_bytes']==16384 for r in records),
        all_captured_requests_require_read_write=all(r['requested_protection']&3==3 for r in records),
        failed_400_by_400_creations=len(re.findall(r'SWEET SURF: backboardd create id 0 400x400 .*? -> 0xe00002c8',text)),
        aegir_sig6_records=len(crashes),aegir_pids=[int(p) for p in crashes],
        source_region_protection_and_max_protection_not_yet_observed=True,
        exact_exception_cause_not_yet_observed=True,complete_panel_and_peripheral_goal_not_achieved=True)


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True)
    a=p.parse_args();report=inspect(a.directory);out=Path(a.output);assert not out.exists()
    out.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:report[k] for k in ('token','memory_entry_record_count',
        'all_captured_errors_are_KERN_PROTECTION_FAILURE','all_captured_maps_are_16KiB',
        'failed_400_by_400_creations','aegir_sig6_records')},indent=2))
