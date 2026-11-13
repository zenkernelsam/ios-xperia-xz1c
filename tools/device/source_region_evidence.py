"""Verify actual pre-init source permissions and their following memory-entry errors."""
import argparse,json,re
from pathlib import Path
from trial_evidence import verify

def inspect(directory):
    root=Path(directory);proof=verify(root,'gpu',version=726)
    text=(root/'cycle-01/serial.raw.log').read_text(errors='replace')
    start=text.index('SWEET LOOP LIVE proto=1 token='+proof['token'])
    end=text.index('SWEET LOOP ACK FASTBOOT token='+proof['token'])
    text=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',text[start:end])
    samples=[]
    expression=r'source region begin group=(\d+) call=(\d+) width=(\d+) height=(\d+) source=(0x[0-9a-f]+) raw=(\d+) start=(0x[0-9a-f]+) end=(0x[0-9a-f]+) metadata_only=1'
    for begin in re.finditer(expression,text):
        group,call,width,height,source,raw,low,high=begin.groups();g=str(group);n=str(call)
        def one(pattern):
            rows=list(re.finditer(pattern,text));assert len(rows)==1,pattern;return rows[0]
        finish=one(r'source region end group='+g+r' call='+n+r' regions=(\d+) result=0 complete=1 metadata_only=1')
        assert begin.end()<finish.start();scope=text[begin.end():finish.start()]
        items=list(re.finditer(r'source region item group='+g+r' call='+n+r' index=(\d+) queried=(0x[0-9a-f]+) address=(0x[0-9a-f]+) size=(\d+) kr=0 prot=([0-9a-f]+) max=([0-9a-f]+) tag=(\d+) share=(\d+) submap=0 depth=(\d+) covered=1',scope))
        assert items and int(finish[1])==len(items)
        cursor=int(low,16);limit=int(high,16);regions=[]
        for index,item in enumerate(items):
            assert int(item[1])==index and int(item[2],16)==cursor
            address=int(item[3],16);size=int(item[4]);assert address<=cursor<address+size
            cursor=min(limit,address+size)
            regions.append(dict(address=item[3],bytes=size,protection=int(item[5],16),
                max_protection=int(item[6],16),tag=int(item[7]),share=int(item[8]),depth=int(item[9])))
        assert cursor==limit
        properties={m[1]:dict(kind=m[2],number=int(m[3])) for m in re.finditer(
            r'source property group='+g+r' call='+n+r' key=(\S+) kind=(\S+) number=(-?\d+)',scope)}
        assert properties['IOSurfaceAddress']['number']==int(source,16)
        samples.append(dict(group=int(group),call=int(call),width=int(width),height=int(height),source=source,
            raw_bytes=int(raw),coverage_bytes=limit-int(low,16),regions=regions,properties=properties,
            position=begin.start(),end_position=finish.end()))
    posters=[s for s in samples if s['group']==1]
    assert len(posters)==12 and all((s['width'],s['height'],s['raw_bytes'],s['coverage_bytes'])==(400,400,640000,655360) for s in posters)
    assert all(all(r['protection']==r['max_protection']==1 and r['tag']==51 for r in s['regions']) for s in posters)
    readwrite=[s for s in samples if all(r['protection']&3==3 for r in s['regions'])]
    assert readwrite and any((s['width'],s['height'])==(750,1624) for s in readwrite)
    for sample in posters:
        aligned=int(sample['source'],16)&~16383
        assert re.search(r'SWEET SURF MEMREF ERROR address='+hex(aligned)+r' requested=655360 actual=655360 prot=0x100003 page_mask=0x3fff kr=2',text[sample['end_position']:])
    for sample in samples:sample.pop('position');sample.pop('end_position')
    return dict(token=proof['token'],kernel_sha256=proof['kernel_sha256'],image_sha256=proof['image_sha256'],
        log_sha256=proof['log_sha256'],ram_library_binding=proof['ram_library_binding'],strict_return=proof['return_complete'],
        pre_init_regions_observed=True,poster_source_current_and_max_protection_are_read_only=True,
        poster_tag_is_VM_MEMORY_LAYERKIT=True,poster_creation_requires_read_write_and_fails=True,
        readwrite_shared_selftest_pixels_passed=True,source_samples=samples,
        caller_memory_mutation_or_permission_changes=0,read_only_surface_implementation_not_yet_verified=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    report=inspect(a.directory);target=Path(a.output);assert not target.exists()
    target.write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:report[k] for k in ('token','poster_source_current_and_max_protection_are_read_only','poster_tag_is_VM_MEMORY_LAYERKIT','readwrite_shared_selftest_pixels_passed')}))
