"""Actual tested GLES pixel/lifetime checks, consolidated from the v724 verifiers.

All checks consume the current LIVE-to-ACK scope or that boot's final close.
They do not imply complete panel UI, audio or wireless acceptance.
"""
import re

def only(pattern, text):
    found = list(re.finditer(pattern, text)); assert len(found) == 1, pattern
    return found[0]

def expected_writer_hash():
    h = 14695981039346656037
    colours = ((0, 0, 255, 255), (0, 255, 0, 255), (255, 0, 0, 255), (255, 255, 255, 255))
    for y in range(1624):
        for x in range(750):
            for value in colours[(y >= 812) * 2 + (x >= 375)]: h = ((h ^ value) * 1099511628211) & 0xffffffffffffffff
    return f"{h:016x}"

def refresh_hash(width,height,bpe,phase):
    value=14695981039346656037
    for y in range(height):
        for x in range(width):
            if bpe==2:
                light=(x*17+phase*59+3)&255;pixels=(light,light,light,(x*7+phase*43+31)&255)
            else:pixels=((x*3+y*5+phase*37)&255,(x*11+y*7+phase*53)&255,(x*13+y*17+phase*71)&255,(x+y+phase*19+127)&255)
            for p in pixels:value=((value^p)*1099511628211)&0xffffffffffffffff
    return f"{value:016x}"

def state_hash(selected,changed):
    width,height=(9,3) if selected else (7,5);value=14695981039346656037
    for y in range(8):
        for x in range(8):
            tx=((2*x+1)*width)//16;ty=((2*y+1)*height)//16
            pixel=[(tx*17+ty*23+selected*79+31)&255,(tx*7+ty*13+changed*113+47)&255,(tx*11+ty*5+selected*67+101)&255,255]
            if not selected:pixel=[pixel[3],pixel[2],pixel[1],pixel[0]]
            for byte in pixel:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'

def frame(selected):
    w,h=(9,3) if selected else (7,5);pixels=[]
    for y in range(8):
        for x in range(8):
            sx=(2*x+1)*w//16;sy=(2*y+1)*h//16
            pixels.extend(((17*sx+23*sy+79*selected+31)&255,(7*sx+13*sy+47)&255,(11*sx+5*sy+67*selected+101)&255,255))
    return bytes(pixels)

def hash_bytes(data,value=14695981039346656037):
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return value

def verify_core(scope, report):
    text=scope
    ready = only(r"SWEET SCATTER READY END result=0 context=1 identity=1 idle=1 input_bits=36 output_bits=36 native_probe=0", text)
    triangle = only(r"full triangle width=750 height=1624 bytes=4872000 checked=1215626 mismatches=0 fbo=8cd5 linked=1 err=0 hash=e170cd3c625c4a45", text)
    texture = only(r"full texture width=750 height=1624 bytes=4872000 checked=1218000 mismatches=0 linked=1 err=0 hash=ed689397267e7cf5", text)
    sampler = only(r"full surface sampler id=(\d+) handle=(\d+) width=750 height=1624 pitch=3072 bytes=4988928 checked=1218000 mismatches=0 fbo=8cd5 linked=1 err=0 hash=ed689397267e7cf5", text)
    writer = only(r"full surface writer id=(\d+) handle=(\d+) width=750 height=1624 pitch=3072 checked=1218000 mismatches=0 fbo=8cd5 err=0 sync=0 lock=0 hash=([0-9a-f]{16})", text)
    end = only(r"full surface selftest -> PASS phase=writer", text)
    assert ready.start() < triangle.start() < texture.start() < sampler.start() < writer.start() < end.start()
    assert sampler.group(1, 2) == writer.group(1, 2)  # identical real IOSurface ID and GEM handle
    assert writer[3] == expected_writer_hash()
    imported = only(r"actual surface import id=" + sampler[1] + r" kr=0 count=3 handle=" + sampler[2] +
        r" gpu=(0x[0-9a-f]+) bytes=4988928 ret=0", text)
    assert ready.start() < imported.start() < sampler.start()
    assert 0x40000000 <= int(imported[1], 0) < 0x80000000 and int(imported[1], 0) % 16384 == 0
    report.update(canvas=[750,1624], triangle_checked=1215626, texture_checked=1218000,
                  surface_id=int(sampler[1]), gem_handle=int(sampler[2]),
                  surface_pixels_each_direction=1218000, sampler_hash=texture[0].split('hash=')[-1],
                  writer_bgra_hash=writer[3], mismatches=0)
    return report

def verify_retirement(text, token, report):
    ack=only(r"SWEET LOOP ACK FASTBOOT token="+token,text)
    closed=list(re.finditer(r"SWEET SCATTER CLOSE owner=(\d+) live=(\d+) allocated=(\d+) released=(\d+) unmapped=(\d+) orphaned=(\d+) active=(\d+) root=(\d+) mapper=(\d+)",text))
    assert closed and closed[-1].start()>ack.start()
    last=closed[-1];values=list(map(int,last.groups()))
    owner,live,allocated,released,unmapped,orphaned,active,root_visible,mapper=values
    assert owner>0 and live==0 and allocated>0 and allocated==released and 0<unmapped<=allocated
    assert orphaned==active==root_visible==0 and mapper==1
    assert "SWEET SCATTER ORPHAN " not in text
    reset=text.index("SWEET LOOP RESET pon=0x06",ack.end());assert last.end()<reset
    report['final_close']=dict(owner=owner,live=live,allocated=allocated,released=released,
        unmapped=unmapped,orphaned=orphaned,active=active,root_visible=root_visible,mapper=mapper)
    return report

def verify_caller(text, scope, token, report):
    live=only(r"SWEET LOOP LIVE proto=1 token="+token,text)
    ack=only(r"SWEET LOOP ACK FASTBOOT token="+token,text)
    mutate=only(r"caller mutate id=(\d+) width=750 height=1624 pitch=3072 offset=128 requested=(0x[0-9a-f]+) returned=(0x[0-9a-f]+) before_wrong=1218000 before_hash=([0-9a-f]{16}) checked=1218000 mismatches=0 hash=([0-9a-f]{16}) after_create=1",text)
    ident=mutate[1];assert mutate[2]!=mutate[3] and (int(mutate[2],0)&16383)==(int(mutate[3],0)&16383)==128
    assert mutate[4]!=mutate[5]
    opened=only(r"SWEET SURF CALLER OPEN id="+ident+r" width=750 height=1624 bpr=3072 raw=4988928 offset=128 cover=4997120 required=4988856 status=0",text)
    sampler=only(r"caller surface sampler id="+ident+r" handle=(\d+) width=750 height=1624 pitch=3072 bytes=4988928 checked=1218000 mismatches=0 fbo=8cd5 linked=1 err=0 hash=ed689397267e7cf5",text)
    imported=only(r"actual surface import id="+ident+r" kr=0 count=3 handle="+sampler[1]+r" gpu=(0x[0-9a-f]+) bytes=4988928 ret=0",text)
    assert 0x40000000<=int(imported[1],0)<0x80000000 and int(imported[1],0)&16383==128
    writer=only(r"caller surface writer id="+ident+r" handle="+sampler[1]+r" width=750 height=1624 pitch=3072 checked=1218000 mismatches=0 fbo=8cd5 err=0 sync=0 lock=0 hash=93a9cad43faf6b45",text)
    original=only(r"caller original writer id="+ident+r" checked=1218000 mismatches=0 hash=93a9cad43faf6b45 source_vm_live=1",text)
    unmapped=only(r"caller source vm release id="+ident+r" bytes=4997120 result=0 held_surface=1 held_gpu=1",text)
    held=only(r"caller held sampler id="+ident+r" handle="+sampler[1]+r" width=750 height=1624 checked=1218000 mismatches=0 err=0 hash=ed689397267e7cf5 source_vm_released=1",text)
    end=only(r"caller selftest -> PASS phase=held_sampler",text)
    assert live.start()<opened.start()<mutate.start()<imported.start()<sampler.start()<writer.start()<original.start()<unmapped.start()<held.start()<end.start()<ack.start()
    closes=list(re.finditer(r"SWEET SURF CALLER CLOSE id=(\d+) bytes=(\d+) status=(\d+) released=(\d+) active=(\d+) outstanding=(\d+) retained=(\d+)",text))
    assert closes and any(m[1]==ident and m[3]=="0" and m[4]=="1" for m in closes)
    assert not re.search(r"SWEET SURF CALLER CLOSE .*status=[1-9]",text)
    last=closes[-1];assert last.start()>ack.start() and last.groups()[2:]==("0","1","0","0","0")
    reset=text.index("SWEET LOOP RESET pon=0x06",ack.end());assert last.end()<reset
    report['caller_surface_id']=int(ident)
    report['final_caller_close']=dict(id=int(last[1]),bytes=int(last[2]),status=0,released=1,active=0,outstanding=0,retained=0)
    report['caller_pixels_each_check']=1218000
    return report

def verify_refresh(text, scope, report):
    cases=[]
    for width,height,bpe,pitch,raw in ((256,1,2,672,512),(60,60,4,240,14400),(22,27,4,88,2376)):
        frames=[];ident=None;previous_seed=None;previous_pos=-1
        for phase in range(4):
            match=only(r"copy refresh draw id=(\d+) width="+str(width)+r" height="+str(height)+r" bpe="+str(bpe)+r" pitch="+str(pitch)+r" raw="+str(raw)+r" phase="+str(phase)+r" seed=(\d+) uploads="+str(phase+1)+r" checked="+str(width*height)+r" mismatches=0 fbo=8cd5 err=0 hash=([0-9a-f]{16}) source_vm_released="+str(phase==3 and 1 or 0),scope)
            ident=ident or match[1];assert ident==match[1] and match.start()>previous_pos
            seed=int(match[2]);assert previous_seed is None or previous_seed!=seed
            expected=refresh_hash(width,height,bpe,phase);assert match[3]==expected
            unchanged=only(r"copy refresh unchanged id="+ident+r" phase="+str(phase)+r" seed="+str(seed)+r" uploads="+str(phase+1)+r" result=1 checked="+str(width*height)+r" mismatches=0 err=0 hash="+expected,scope)
            assert match.start()<unchanged.start()
            if phase==3:
                release=only(r"copy refresh source release id="+ident+r" width="+str(width)+r" height="+str(height)+r" bytes=16384 result=0 held_surface=1 held_copy=1",scope)
                assert previous_pos<release.start()<match.start()
            frames.append({"phase":phase,"seed":seed,"uploads":phase+1,"pixels":width*height,"mismatches":0,"hash":expected,"unchanged_skip_verified":True})
            previous_seed=seed;previous_pos=unchanged.end()
        end=only(r"copy refresh case -> PASS width="+str(width)+r" height="+str(height)+r" bpe="+str(bpe)+r" pitch="+str(pitch)+r" phase=complete uploads=4",scope)
        assert previous_pos<end.start()
        assert re.search(r"SWEET SURF CALLER OPEN id="+ident+r" width="+str(width)+r" height="+str(height)+r" bpr="+str(pitch)+r" raw="+str(raw)+r" offset=0 cover=16384 required="+str(raw)+r" status=0",scope)
        cases.append({"id":int(ident),"width":width,"height":height,"bpe":bpe,"pitch":pitch,"raw_bytes":raw,"frames":frames,"source_vm_release_verified":True})
    assert "copy refresh selftest -> PASS" in scope and "copy refresh selftest -> WRONG" not in text
    report.update(actual_dynamic_small_surface_refresh_verified=True,actual_unchanged_seed_skips_upload=True,
        actual_copy_after_original_vm_release_verified=True,refresh_cases=cases,
        shared_production_snapshot_and_gl_transfer_core=True,panel_ui_colour_fps_not_verified=True)
    return report

def verify_reuse(text, scope, report):
    case_counts=[];common_context=None
    for case in report["refresh_cases"]:
        ident=str(case["id"]);context=None
        for phase in range(4):
            operation=only(r"copy storage id="+ident+r" phase="+str(phase)+r" context=(\d+) images=1 subimages="+str(phase)+r" uploads="+str(phase+1),scope)
            assert int(operation[1])>0
            context=context or int(operation[1]);assert context==int(operation[1])
        assert only(r"copy storage case id="+ident+r" images=1 subimages=3",scope)
        common_context=common_context or context;assert context==common_context
        case_counts.append({"surface_id":case["id"],"context":context,"images":1,"subimages":3})
    steps=[];owner_a=owner_b=texture=None;last_position=-1
    for step in range(4):
        hash_value=refresh_hash(8,8,4,2 if step==3 else 1)
        record=only(r"copy context step="+str(step)+r" context=(\d+) texture=(\d+) images=1 subimages="+str(int(step==3))+r" uploads="+str(step+1)+r" checked=64 mismatches=0 fbo=8cd5 err=0 hash="+hash_value,scope)
        assert record.start()>last_position;last_position=record.end()
        owner=int(record[1]);name=int(record[2]);assert owner>0 and name>0
        if step==0:owner_a=owner;texture=name
        elif step==1:owner_b=owner;assert owner_b!=owner_a and name==texture
        else:assert owner==owner_a and name==texture
        steps.append({"step":step,"context":owner,"texture":name,"images":1,"subimages":int(step==3),"checked":64,"mismatches":0,"hash":hash_value})
    assert "copy context selftest -> PASS" in scope and "copy context selftest -> WRONG" not in text
    report.update(actual_owned_texture_storage_reuse_verified=True,actual_different_context_same_texture_name_verified=True,
        dynamic_storage_counts=case_counts,texture_image_definitions=3,texture_subimage_updates=9,
        context_switch_steps=steps,physical_allocation_reduction_or_ui_fps_not_inferred=True,
        ca_all_context_and_thread_state_not_verified=True)
    return report

def verify_state(text, scope, report):
    steps=[];owners={};texture=program=None;position=-1
    for step in range(6):
        i=step&1;changed=int(step>=4);w,h=(9,3) if i else (7,5)
        pattern=r'sweetgl: bridge state step='+str(step)+r' context=(\d+) texture=(\d+) program=(\d+) width='+str(w)+r' height='+str(h)+r' images=1 updates='+str(changed)+r' checked=64 mismatches=0 err=0 hash='+state_hash(i,changed)
        matches=list(re.finditer(pattern,scope));assert len(matches)==1,pattern;m=matches[0]
        assert m.start()>position;position=m.end();owner=int(m[1]);name=int(m[2]);prog=int(m[3]);assert owner and name and prog
        if step<2:owners[i]=owner
        assert owners[i]==owner
        texture=texture or name;program=program or prog;assert name==texture and prog==program
        steps.append({'step':step,'context':owner,'texture':name,'program':prog,'width':w,'height':h,'definitions':1,'updates':changed,'pixels':64,'mismatches':0,'hash':state_hash(i,changed)})
    assert owners[0]!=owners[1] and 'sweetgl: bridge state selftest -> PASS' in scope
    assert 'sweetgl: bridge state selftest -> WRONG' not in text
    report.update(actual_production_gli_context_state_pixels_verified=True,bridge_state_steps=steps,
        actual_rectangle_program_and_argb_swizzle_isolated=True,actual_distinct_surface_copy_registry_isolated=True,
        bridge_pixels_checked=384,live_simultaneous_thread_rendering_not_verified=True,
        shared_eagl_contexts_and_shader_name_reuse_not_verified=True)
    return report

def verify_lifecycle(text, scope, report):
    value=14695981039346656037;marks=[];owner=shader=program=None;last=-1
    for step in range(160):
        selected=(step//2)&1;rectangle=int(step%2==0)
        value=hash_bytes(frame(selected),value);value=hash_bytes(frame(selected^1),value)
        if step%16 and step!=159:continue
        pattern=r'program life cycle step='+str(step)+r' context=(\d+) shader=(\d+) program=(\d+) rectangle='+str(rectangle)+r' shaders='+str(rectangle)+r' programs='+str(rectangle)+r' mask='+format(0x8000|(1<<(selected^1)),'x')+r' draws='+str((step+1)*2)+r' pixels='+str((step+1)*128)+r' mismatches=0 hash='+f'{value:016x}'
        matches=list(re.finditer(pattern,scope));assert len(matches)==1,pattern;m=matches[0]
        owner=owner or int(m[1]);shader=shader or int(m[2]);program=program or int(m[3]);assert owner and shader and program
        assert (int(m[1]),int(m[2]),int(m[3]))==(owner,shader,program) and m.start()>last;last=m.end()
        marks.append({'cycle':step,'context':owner,'shader':shader,'program':program,'draws':(step+1)*2,'pixels':(step+1)*128,'hash':f'{value:016x}'})
    def one(pattern):
        matches=list(re.finditer(pattern,scope));assert len(matches)==1,pattern;return matches[0]
    h0=f'{hash_bytes(frame(0)):016x}';h1=f'{hash_bytes(frame(1)):016x}'
    staged=one(r'program life staged compiled=1 checked=64 mismatches=0 hash='+h1)
    failure=one(r'program life failed_link cache_retained=1 mask=8002 gpu_draw_test=0')
    ds=one(r'program life deferred_shader alive=1 checked=64 mismatches=0 hash='+h1)
    dp=one(r'program life deferred_program alive=1 checked=64 mismatches=0 hash='+h1)
    recycled=one(r'program life recycled shader='+str(shader)+r' program='+str(program)+r' same_names=1 shaders=0 programs=0 fetches=0 checked=64 mismatches=0 hash='+h0)
    final=one(r'program life selftest -> PASS cycles=160 draws=320 pixels=20480 max_shaders=1 max_programs=1 extra_draws=4 hash='+f'{value:016x}')
    assert last<staged.start()<failure.start()<ds.start()<dp.start()<recycled.start()<final.start()
    enabled=one(r'sweetgl: shader names context='+str(owner)+r' fresh=1 reuse=1')
    retired=one(r'program life retired shader='+str(shader)+r' program='+str(program)+r' shader_alive=0 program_alive=0')
    new_names=one(r'program life new_names old_shader='+str(shader)+r' old_program='+str(program)+r' shader='+str(shader)+r' program='+str(program)+r' same_names=1')
    assert enabled.start()<scope.index('program life cycle step=0')
    assert dp.end()<retired.start()<new_names.start()<recycled.start()
    assert not re.search(r'shader names context=\d+ fresh=1 reuse=0',scope)
    assert 'program life selftest -> WRONG' not in text and 'shader lifecycle capacity exhausted' not in text
    report.update(actual_program_lifecycle_and_relinks_verified=True,actual_same_shader_and_program_160_relinks=True,
        actual_sampler_scalar_and_vector_mask_refresh_verified=True,actual_staged_source_keeps_compiled_shader_verified=True,
        actual_deferred_shader_and_program_delete_draws_verified=True,actual_recycled_shader_and_program_names_verified=True,
        actual_failed_link_bridge_cache_retention_verified=True,failed_link_gpu_rendering_not_verified=True,
        actual_fresh_private_mesa_name_reuse_enabled=True,actual_native_retirement_before_reuse_verified=True,
        lifecycle_draws=320,lifecycle_pixels=20480,lifecycle_extra_draw_pixels=256,
        lifecycle_hash=f'{value:016x}',lifecycle_checkpoints=marks,shader_and_program_table_peaks=1,
        all_object_limits_and_sampler_arrays_not_verified=True,hardware_frame_rate_not_inferred=True)
    return report

def verify(text, token):
    live=only(r'SWEET LOOP LIVE proto=1 token='+token,text)
    ack=only(r'SWEET LOOP ACK FASTBOOT token='+token,text)
    scope=text[live.start():ack.start()]
    assert not re.search(r'selftest -> WRONG|mismatches=[1-9]|shader lifecycle capacity exhausted',scope)
    assert 'SWEET GPU: scatter timeout ' not in text and 'SWEET SCATTER ORPHAN ' not in text
    report=verify_core(scope,{})
    verify_retirement(text,token,report)
    verify_caller(text,scope,token,report)
    verify_refresh(text,scope,report)
    verify_reuse(text,scope,report)
    verify_state(text,scope,report)
    verify_lifecycle(text,scope,report)
    report['panel_ui_colour_fps_not_verified']=True
    report['audio_wireless_not_verified']=True
    return report
