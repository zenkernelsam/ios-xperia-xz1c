"""Verify real r3 BO IOVA, offset comparison and exact erroneous page-origin pixels."""
import argparse,json,re
from pathlib import Path
from failed_trial_evidence import verify as verify_failure
from readonly_pixels import expected_hash,only
from ram_library import fnv

def inspect(directory):
    root=Path(directory);proof=verify_failure(root)
    raw=(root/'cycle-01/serial.raw.log').read_text(errors='replace')
    token=proof['token'];start=raw.index('SWEET LOOP LIVE proto=1 token='+token);end=raw.index('SWEET LOOP ACK FASTBOOT token='+token)
    text=re.sub(r'\r?\n(?!SWEET|CRASH|Process|\||exec|IOK|MESA)','',raw[start:end])
    source=bytearray(655360)
    for y in range(400):
        for x in range(400):
            at=32+y*1600+x*4
            source[at:at+4]=bytes(((x^y)&255,(x*5+y*11)&255,(x*17+y*3)&255,255))
    wrong=bytearray(source[:640000])
    for at in range(0,len(wrong),4):wrong[at],wrong[at+2]=wrong[at+2],wrong[at]
    erroneous=fnv(wrong);cases=[]
    for offset in (32,64,128):
        views=list(re.finditer(r'readonly view id=(\d+) width=400 height=400 source=(0x[0-9a-f]+) returned=(0x[0-9a-f]+) region=0 prot=1 max=1 write_lock=e00002cf read_lock=0 checked=160000 mismatches=0 hash='+expected_hash(True),text))
        views=[v for v in views if int(v[2],16)&16383==offset];assert len(views)==1
        view=views[0];ident=view[1];assert int(view[3],16)&16383==offset
        imported=only(r'probe Mesa import id='+ident+r' handle=(\d+) bo_handle=(\d+) bo_iova=(0x[0-9a-f]+) stride=1600 pitch=1600 offset=0 layer=0 size=640000 refs=1',text)
        assert imported[1]==imported[2] and int(imported[3],16)&16383==offset
        mismatches=160000 if offset==32 else 0;digest=erroneous if offset==32 else expected_hash()
        first=only(r'readonly sampler id='+ident+r' handle='+imported[1]+r' phase=0 checked=160000 mismatches='+str(mismatches)+r' fbo=8cd5 linked=1 err=0 hash='+digest+r' source_vm_released=0',text)
        assert view.start()<imported.start()<first.start()
        if offset==32:
            assert 'readonly case offset=32 -> WRONG phase=sampler' in text
        else:
            held=only(r'readonly sampler id='+ident+r' handle='+imported[1]+r' phase=1 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+digest+r' source_vm_released=1',text)
            assert first.start()<held.start()
            assert 'readonly case offset='+str(offset)+' -> PASS phase=held_sampler' in text
        cases.append(dict(offset=offset,surface_id=int(ident),gem_handle=int(imported[1]),bo_iova=imported[3],
            cpu_hash=expected_hash(True),gpu_hash=digest,mismatches=mismatches,after_source_vm_release_passed=offset!=32))
    return dict(actual_failure=proof,offset_cases=cases,offset32_gpu_equals_independent_page_origin_hash=True,
        independent_erroneous_hash=erroneous,mesa_bo_iova_keeps_offset=True,
        hardware_internal_truncation_stage_not_yet_observed=True,full_goal_not_complete=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    result=inspect(a.directory);out=Path(a.output);assert not out.exists();out.write_bytes((json.dumps(result,indent=2)+'\n').encode())
    print(json.dumps({'independent_erroneous_hash':result['independent_erroneous_hash'],'offset_cases':result['offset_cases']},indent=2))
