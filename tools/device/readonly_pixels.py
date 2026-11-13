"""Actual read-only caller-view permissions, 160k GPU pixels and original-VM release."""
import re
from functools import lru_cache
def only(pattern,text):
    found=list(re.finditer(pattern,text));assert len(found)==1,pattern;return found[0]
@lru_cache(maxsize=2)
def expected_hash(bgra=False):
    value=14695981039346656037
    for y in range(400):
        for x in range(400):
            pixels=[(x*17+y*3)&255,(x*5+y*11)&255,(x^y)&255,255]
            if bgra:pixels[0],pixels[2]=pixels[2],pixels[0]
            for p in pixels:value=((value^p)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'
def verify(text,token):
    live=only('SWEET LOOP LIVE proto=1 token='+token,text)
    ack=only('SWEET LOOP ACK FASTBOOT token='+token,text)
    scope=text[live.start():ack.start()]
    if 'production sampler id=' in scope:
        return verify_production(scope)
    view=only(r'readonly view id=(\d+) width=400 height=400 source=(0x[0-9a-f]+) returned=(0x[0-9a-f]+) region=0 prot=1 max=1 write_lock=e00002cf read_lock=0 checked=160000 mismatches=0 hash='+expected_hash(True),scope)
    ident=view[1];assert view[2]!=view[3]
    assert int(view[2],16)&16383==int(view[3],16)&16383==32
    opened=only(r'SWEET SURF READONLY id='+ident+r' width=400 height=400 bytes=640000 source='+view[2]+r' direction=out cpu_write=0 gpu_write=0',scope)
    begin=only(r'readonly sampler id='+ident+r' handle=(\d+) phase=0 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+expected_hash()+r' source_vm_released=0',scope)
    held=only(r'readonly sampler id='+ident+r' handle='+begin[1]+r' phase=1 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+expected_hash()+r' source_vm_released=1',scope)
    finish=only('readonly selftest -> PASS phase=held_sampler',scope)
    assert opened.start()<view.start()<begin.start()<held.start()<finish.start()
    return dict(surface_id=int(ident),gem_handle=int(begin[1]),view_read_only=True,write_lock_refused=True,
        pixels_per_gpu_check=160000,source_vm_removed_with_gpu_and_surface_held=True,
        bgra_cpu_hash=expected_hash(True),rgba_gpu_hash=expected_hash(),mismatches=0,
        cpu_or_gpu_write_into_readonly_memory_not_attempted=True)

def verify_production(scope):
    cases=[];last=-1
    for offset in (32,64,128):
        views=list(re.finditer(r'readonly view id=(\d+) width=400 height=400 source=(0x[0-9a-f]+) returned=(0x[0-9a-f]+) region=0 prot=1 max=1 write_lock=e00002cf read_lock=0 checked=160000 mismatches=0 hash='+expected_hash(True),scope))
        views=[v for v in views if int(v[2],16)&16383==offset]
        assert len(views)==1,'Missing or duplicate readonly view at offset '+str(offset)
        view=views[0];ident=view[1]
        assert int(view[3],16)&16383==offset and view[2]!=view[3] and view.start()>last
        # This message is emitted before Surface receives its published id.
        opened=only(r'SWEET SURF READONLY id=0 width=400 height=400 bytes=640000 source='+view[2]+r' direction=out cpu_write=0 gpu_write=0',scope)
        path='copy' if offset==32 else 'shared'
        binds=list(re.finditer(r'production sampler id='+ident+r' path='+path+r' offset='+str(offset)+r' pitch=1600 result=1',scope))
        assert len(binds)==3 and all(v.start()>view.start() for v in binds)
        begin=only(r'readonly sampler id='+ident+r' offset='+str(offset)+r' phase=0 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+expected_hash()+r' source_vm_released=0',scope)
        held=only(r'readonly sampler id='+ident+r' offset='+str(offset)+r' phase=1 checked=160000 mismatches=0 fbo=8cd5 linked=1 err=0 hash='+expected_hash()+r' source_vm_released=1',scope)
        finish=only(r'readonly case offset='+str(offset)+r' -> PASS phase=held_sampler',scope)
        assert opened.start()<view.start()<binds[0].start()<binds[1].start()<begin.start()<binds[2].start()<held.start()<finish.start()
        last=finish.end()
        cases.append(dict(surface_id=int(ident),offset=offset,path=path,pixels_per_gpu_check=160000,
            cpu_view_readonly=True,write_lock_refused=True,source_vm_removed_with_surface_held=True,
            cpu_bgra_hash=expected_hash(True),gpu_rgba_hash=expected_hash(),mismatches=0))
    finish=only('readonly selftest -> PASS phase=held_sampler',scope)
    assert last<finish.start()
    return dict(production_bind_surface_verified=True,cases=cases,total_gpu_pixels=960000,
                unaligned_source_uses_seed_refresh_upload=True,aligned_source_uses_direct_shared_pages=True,
                writes_into_original_readonly_pages_not_attempted=True,full_panel_and_peripheral_goal_not_complete=True)
