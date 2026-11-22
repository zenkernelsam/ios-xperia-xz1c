"""Build a one-shot trial from exact tested baseline inputs, never from a legacy Patch path."""
import argparse,hashlib,json,re,secrets,struct,sys,time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'));import adt,mkpayload
from diagnostic_candidate import configuration
from ram_library import load as load_ram
from native_proof import validate as validate_native,digest as native_digest
from controller import Artifact,SHIM_SHA,SCREEN_CAPTURE_TIMES,checked_screen_capture_after,swipe_once_binding
from numeric_boot_args import add_sampler
from serial_worker import capture_binding
from display_geometry_binding import transform as display_transform,arguments as display_arguments,compact as display_compact
from native_grid_control import apply as apply_grid_control,validate as validate_grid_control
def sha(data):return hashlib.sha256(data).hexdigest()
def observer_environment(word,target):
    assert word.startswith('sweetenv=') and target in ('AegirPoster','backboardd')
    word=word.replace(',SWEETGL_TRACE=1','')
    if target=='AegirPoster':return word+';AegirPoster:DYLD_INSERT_LIBRARIES=/dev/sglram/e'
    original='backboardd:DYLD_INSERT_LIBRARIES=/dev/sglram/m'
    assert word.count(original)==1 and 'AegirPoster:' not in word
    return word.replace(original,original+':/dev/sglram/e')
BASE_HASHES={'private':'b958bd29a457cae61be27a53e1237ccdc4c1247d4cbcff238afbe34d88615162',
             'ready':'2eb112563f920669e2e932799c56b6af03e79e09a1c8e73833855a4ac1f93af1',
             'gpu':'ee284994e2ac7e280f0a3b727ce1ebb4796e3813bbd98b524fa16e6c3d5f9082'}
BASE_KERNEL='da6ddc44fc074eb393f135e1395798483fe2a52eb420dfecea883e0833136c9a'

def assemble_image(header,shim,payload):
    result=bytearray(header);struct.pack_into('<I',result,16,len(payload))
    result+=shim+b'\0'*(-len(shim)%4096);result+=payload+b'\0'*(-len(payload)%4096)
    return bytes(result)

def prove_payload(record):
    """Bind the entire embedded image to the compiled kernel and exact tested input blobs."""
    mode=record['trial_mode'];version=record['version']
    kernel=ROOT/f'build/surface-diag-v{version}/kernel.v{version}.macho'
    raw_kernel=kernel.read_bytes()
    assert sha(raw_kernel)==record['kernel_sha256'], 'Compiled kernel differs from image record'
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,_,reference,blobs,_,shim=baseline(selected,ROOT/'assets/baselines')
    if mode=='ramcpu':blobs.append(baseline('gpu',ROOT/'assets/baselines')[3][-1])
    if 'ram_library_binding' in record:
        assert mode in ('ramcpu','gpu')
        blob,_,ram_package=load_ram(record['ram_package'],record['ram_library_binding'])
        validate_grid_control(record,ram_package)
        blobs=[(n,blob if n=='GL-RAM-Z' else data) for n,data in blobs]
    elif 'native_grid_control' in record:
        raise ValueError('Native grid control requires its exact RAM package')
    expected_blobs={name:{'bytes':len(data),'sha256':sha(data)} for name,data in blobs}
    assert expected_blobs==record['blobs'], 'Trial firmware/RAM inputs differ from tested baseline'
    if 'display_geometry_binding' in record:
        assert version>=730 and mode in ('ramcpu','gpu') and 'ram_library_binding' in record
        reference,binding=display_transform(reference,record['display_geometry_binding']['profile'])
        assert binding==record['display_geometry_binding'],'Native geometry ADT/binding differs from exact input'
        from numeric_boot_args import numeric_options
        options=numeric_options(record);assert options.get('sf2x')=='0' and options.get('sweetfb_native')=='1'
    payload,_=mkpayload.build(raw_kernel,reference,record['cmdline'],blobs=blobs)
    image=assemble_image(raw[:4096],shim,payload)
    actual=Path(record['image']).read_bytes()
    assert image==actual and sha(image)==record['sha256'], 'Embedded kernel/payload differs from audited inputs'
    return {'image_sha256':sha(image),'kernel_sha256':sha(raw_kernel),'blob_count':len(blobs),
            'complete_payload_reconstructed_from_compiled_kernel':True}
def baseline(name,folder):
    image=folder/({'private':'v724-private.img','ready':'v724-ready.img','gpu':'v724-tested-panel.img'}[name])
    raw=image.read_bytes();assert sha(raw)==BASE_HASHES[name],image
    ks,rs=struct.unpack_from('<I',raw,8)[0],struct.unpack_from('<I',raw,16)[0]
    off=4096+(ks+4095)//4096*4096;payload=raw[off:off+rs]
    assert raw[:8]==b'ANDROID!' and sha(raw[4096:4096+ks])==SHIM_SHA
    assert payload[:8]==b'SWEETXNU' and len(payload)==rs
    vb,entry,ao,an,end,pb,mem=struct.unpack_from('<7Q',payload,16)
    count=struct.unpack_from('<I',payload,12)[0]
    segments=[struct.unpack_from('<4Q',payload,680+i*32) for i in range(count)]
    original_tree=payload[ao:ao+an];tree,parsed=adt.parse(original_tree);assert parsed==an
    chosen=next(c for c in tree.children if c.name=='chosen')
    mmap=next(c for c in chosen.children if c.name=='memory-map')
    names=['GPU-PM4','GPU-PFP','GPU-ZAP-MDT','GPU-ZAP-SEG','GPU-GPMU','ADSP-MDT','ADSP-IMAGE-Z']
    if name=='gpu':names+=['GL-RAM-Z']
    blob_data=[]
    for key in names:
        value=mmap.prop(key);assert value and len(value)==16,key
        pa,bytes=struct.unpack('<2Q',value)
        match=[s for s in segments if s[0]-vb+pb==pa and s[3]==bytes];assert len(match)==1,key
        s=match[0];data=payload[s[2]:s[2]+s[3]];assert len(data)==bytes
        blob_data.append((key,data))
    mmap.props=[v for v in mmap.props if v[0] not in names]
    reference=adt.serialize(tree)
    assert sha(reference)=='257076987411b50f037f70bf97524fae56ac2965e773c1bb8678d05d45480ba6'
    return raw,payload,reference,blob_data,payload[72:680].split(b'\0',1)[0].decode(),raw[4096:4096+ks]
def prepare(mode,kernel,folder,output,version=725,ram_package=None,owner_interval=None,screen_evidence=False,thread_sample=False,screen_after=None,idle_sample=False,swipe_once=False,display_profile=None,native_grid_mode=None):
    assert not (thread_sample and idle_sample), 'Only one reviewed sample trigger can be selected'
    if swipe_once:
        assert mode=='gpu' and screen_evidence and version>=730 and ram_package and screen_after==150
        assert not thread_sample and not idle_sample,'Single-swipe observation stays separate from sampling'
    if screen_after is not None:
        assert mode=='gpu' and screen_evidence, 'Screenshot timing is only for a GPU screen-evidence trial'
        checked_screen_capture_after(screen_after)
    capture_after=(45 if screen_after is None else screen_after) if mode=='gpu' and screen_evidence else None
    config=configuration(version)
    selected='gpu' if mode=='gpu' else 'ready' if mode in ('owner','ready') else 'private'
    raw,payload,reference,blobs,cmd,shim=baseline(selected,folder)
    files=list(blobs)
    # Prove extraction covers the complete tested baseline payload, not just the header.
    reproduced,_=mkpayload.build(str(folder/'kernel-v724-tested.macho'),reference,cmd,blobs=files)
    assert reproduced==payload,'Tested baseline reconstruction mismatch'
    geometry=None
    if display_profile is not None:
        assert display_profile=='xz1c-native-2x' and version>=730 and mode in ('ramcpu','gpu') and ram_package
        assert not thread_sample and not idle_sample,'Geometry and thread sampling are independent trials'
        reference,geometry=display_transform(reference,display_profile)
    token=secrets.token_hex(8);words=cmd.split()
    words=[('sweetloop='+token if w.startswith('sweetloop=') else 'sweetutc='+str(int(time.time())) if w.startswith('sweetutc=') else w) for w in words]
    if mode=='owner':
        words=[w.replace('sweetsample=-','sweetsample=bootdiag').replace('svmro=7','svmro=4') for w in words]
    watchdog=300;observe=240
    if owner_interval is not None:
        assert mode=='owner' and owner_interval==90, 'Only the reviewed owner timing comparison is supported'
        words=[('sweetsample_iv=90' if w.startswith('sweetsample_iv=') else 'sweetar=420' if w.startswith('sweetar=') else w) for w in words]
        watchdog=420;observe=300
    if mode=='ramcpu':
        _,_,_,ramblobs,_,_=baseline('gpu',folder)
        files.append(ramblobs[-1])
    library_binding=None
    if ram_package:
        assert mode in ('ramcpu','gpu')
        blob,library_binding,package=load_ram(ram_package)
        if version>=727:assert 'readonly_probe.c' in package['frontend_build_binding']['source_files']
        files=[(n,blob if n=='GL-RAM-Z' else data) for n,data in files]
        if mode=='gpu' and 'exception_observer' in package:
            # The removed zero-valued scan options have default zero in the
            # audited framebuffer source; TRACE is logging only. Keep cadence.
            words=[w for w in words if w not in ('sweetfb_direct=0','sweetfb_kick=0')]
            words=[observer_environment(w,package['exception_observer']['target_process']) if w.startswith('sweetenv=') else w for w in words]
            # svmro7 owns/defer-starts GPU independently of sweetgpuasync.
            # Keep runtime/display/touch settings; make room for explicit tee.
            words=[w for w in words if w not in ('sweetgpuasync=1','-noprogress','keepsyms=1')]
            words.append('sweetnull=backboardd,AegirPoster')
    if screen_evidence:
        assert version>=730, 'Full screenshots require the separately audited v730 reader'
        # Replace periodic diagnostic dumps with an explicit full snapshot; keep
        # display/GPU/input operation and the existing PON06 return untouched.
        words=[w for w in words if not w.startswith(('sweetfbshot=','sweetfbshot_surface=','sweetfbcolor='))]
        # These zero options have explicit zero defaults in the audited source.
        # Reclaim the fixed payload command area without changing scanout mode.
        words=[w for w in words if w not in ('sweetfb_direct=0','sweetfb_kick=0')]
        words=[('sweetfb_noconsole=1' if w.startswith('sweetfb_noconsole=') else w) for w in words]
        words=[('sweetar=420' if w.startswith('sweetar=') else w) for w in words]
        words.append('sfscreen=1');watchdog=420
    grid_control=None
    if native_grid_mode is not None:
        assert mode=='gpu' and geometry and package['geocompat_build_binding']['revision']==3 and screen_evidence
    if mode=='gpu' and geometry and package.get('geocompat_build_binding',{}).get('revision')==3:
        words,grid_control=apply_grid_control(words,native_grid_mode or 'fitted')
    compact_numeric_ones=[]
    if geometry:words=display_arguments(words)
    if screen_evidence and len(' '.join(words).encode())>607:
        # The pinned XNU PE_parse_boot_argn/getval returns numeric 1 for a
        # present argument without '='. Keep both enabled features identical.
        for name in ('sweetbattery','sf2x'):
            explicit=name+'=1'
            if explicit in words:
                words=[name if w==explicit else w for w in words]
                compact_numeric_ones.append(name)
    if geometry:words=display_compact(words,compact_numeric_ones)
    sample_binding=None
    if thread_sample or idle_sample:
        assert mode=='gpu' and screen_evidence and version>=730 and ram_package
        assert package['exception_observer'].get('registration_only') and package['metal_resources'].get('native_Clear_shader_cache')
        words,sample_binding=add_sampler(words,compact_numeric_ones,'after_library_idle' if idle_sample else 'first_target_clear')
    cmd=' '.join(words);assert len(cmd.encode())<=607
    candidate,info=mkpayload.build(str(kernel),reference,cmd,blobs=files)
    assert b'SWEET SURF CALLER DIAG phase=' in candidate
    if version>=726:
        assert b'SWEET SURF MEMREF ERROR address=' in candidate and b'SWEET SURF MEMREF INIT length=' in candidate
    result=assemble_image(raw[:4096],shim,candidate)
    image=output/'trial.img';assert not image.exists();image.write_bytes(result)
    m={'id':'surface-v'+str(version)+'-'+mode,'version':version,'image':str(image),'sha256':sha(result),
       'kernel_sha256':sha(kernel.read_bytes()),'shim_sha256':SHIM_SHA,'cmdline':cmd,'cmdline_bytes':len(cmd.encode()),
       'token':token,'watchdog_seconds':watchdog,'source_revision':config['binding']['manifest_sha256'],
       'diagnostic_binding':config['binding'],
       'boot_sender':'sony-boot-once','boot_sender_sha256':sha((ROOT/'tools/device/boot_once.py').read_bytes()),
       'return_evidence_sha256':sha((ROOT/'tools/device/return_evidence.py').read_bytes()),
       'trial_mode':mode,'baseline_full_payload_reconstructed':True,
        'serial_capture_binding':capture_binding(token),
       'blobs':{name:{'bytes':len(data),'sha256':sha(data)} for name,data in files}}
    if compact_numeric_ones:m['numeric_one_argument_compaction']=compact_numeric_ones
    if geometry:m['display_geometry_binding']=geometry
    if grid_control:m['native_grid_control']=grid_control
    if sample_binding:m['thread_sample_observation']=sample_binding
    if swipe_once:m['swipe_observation']=swipe_once_binding()
    Artifact(m).validate()
    if library_binding:
        m.update(ram_package=str(Path(ram_package).resolve()),ram_library_binding=library_binding)
        if 'exception_observer' in package:m['exception_observer']=package['exception_observer']
        if 'metal_resources' in package:m['metal_resources']=package['metal_resources']
    if owner_interval is not None:
        m['owner_observation_interval_seconds']=owner_interval
    if compact_numeric_ones:m['numeric_one_argument_compaction']=compact_numeric_ones
    if screen_evidence:
        from scanout_snapshot import binding as screen_binding
        m['screen_snapshot_binding']=screen_binding(m)
        if mode=='gpu':m['screen_capture_after_seconds']=capture_after
    (output/'artifacts.json').write_text(json.dumps({'target_serial':'BH905ND69G','pon_route':6,'artifacts':[m]},indent=2)+'\n')
    common=['SWEET LOOP LIVE proto=1 token='+token,'SWEET RMI4: F12 page=']
    if grid_control:
        common+=['SWEET GEOGRID MODE value='+str(grid_control['value'])]
        if grid_control['value']:common+=['SWEET GEOGRID APPLY native=360x640 old=6 rows=5 cols=4']
    if swipe_once:common+=['SWEET RMI4: swipe up injected']
    if mode in ('private','ramcpu'):
        common+=['SWEET VM RO END snap=2 valid=1 .*writes=0 complete=1',
                 'SWEET SMEM STATE END snap=2 mode=2 attempted=9 .*writes=0 unowned_reads=0 complete=1']
        if mode=='ramcpu':
            library=library_binding or {'library_bytes':14675424,'library_fnv64':'bb763317000ee0d7'}
            common+=['SWEET GLRAM: library read=0 bytes='+str(library['library_bytes'])+' fnv='+library['library_fnv64']+' ro=1']
    elif mode=='owner':
        common+=['SWEET SG OWN END snap=1 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1',
                 'SWEET SG OWN END snap=2 result=0 reads=30 root_reads=512 input_bits=36 output_bits=(36|40) context=1 identity=1 idle=1']
    else:
        common+=['SWEET SCATTER READY END result=0 context=1 identity=1 idle=1 input_bits=36 output_bits=36 native_probe=0']
        if mode=='gpu':
            common+=['sweetgl: full selftest -> PASS','sweetgl: full surface selftest -> PASS',
                     'sweetgl: caller selftest -> PASS','sweetgl: copy refresh selftest -> PASS',
                     'sweetgl: copy context selftest -> PASS',
                     'sweetgl: bridge state selftest -> PASS','sweetgl: program life selftest -> PASS',
                     'SWEET SURF CALLER DIAG phase=.*status=(0x[0-9a-f]+|0)']
            if package['frontend_build_binding'].get('CA_matrix_revision'):
                assert package['frontend_build_binding']['CA_matrix_revision']==1 and package['frontend_build_binding']['GLES_suite_tests']==9
                common+=['SWEET CA MATRIX END result=0 phases=6 checked_bytes=1536 guard_bytes=192 context_disposed=1 production_GLI=1 matrix_slots=2']
            if version>=727:
                assert library_binding,'Read-only trial needs the tested-probe frontend'
                common+=['sweetgl: readonly selftest -> PASS phase=held_sampler']
            if 'exception_observer' in m:
                if m['exception_observer']['target_process']=='backboardd':
                    common+=['SWEET EXCEPTION observer installed process=backboardd limit=16 original_chain=1',
                        'SWEET GLES SUITE END result=0 tests='+str(package['frontend_build_binding'].get('GLES_suite_tests',8))+' state=2 original_tests=1',
                        'SWEET METAL PROBES begin process=backboardd GLES_state=2 full_suite=1 background=1',
                        'SWEET METAL PROBES END process=backboardd result=0 prerequisite=0 full_suite=1 background=1']
                    if m['exception_observer'].get('AIR_sampler_probe'):
                        common+=['SWEET AIR SAMPLER END result=0 checked_bytes=147456 phases=3 final_allocated=0 GPU_only=1']
                    if m['exception_observer'].get('program_cache_probe'):
                        common+=['SWEET PROGRAM CACHE roundtrip phase=2 first_state=[12] restored_state=1 fresh_pipeline=1 valid=1']
                    if m['exception_observer'].get('program_cache_prime'):
                        common+=['SWEET PROGRAM CACHE PRIME END result=0 compiled=[0-9]+ failed=0 max_entries=8 original_requests=1']
                else:
                    common+=['SWEET EXCEPTION observer installed process=AegirPoster limit=16 original_chain=1']
                    if not m['exception_observer'].get('registration_only'):
                        common+=['SWEET EXCEPTION begin call=1 class=.*name=',
                                 'SWEET EXCEPTION end call=1 frames=[1-9].*original_chain=1']
                if 'renderer_observations_max' in m['exception_observer']:
                    common+=['SWEET METAL observer installed renderer=NUNIAegirRenderer limit=32 index_type=[Qq] device_object=1 original_forwarded=1']
                    if not m['exception_observer'].get('original_Metal_registry_probe'):
                        common+=['SWEET METAL uniform call=1 index=[0-2] device_nil=[01].*metadata_only=1 original_called=1']
                if m['exception_observer'].get('original_postdraw_observer'):
                    common+=['SWEET POSTDRAW observer installed=1 interpose_pairs=4 limit=64 metadata_only='+('0 original_arguments_results=0' if m['exception_observer'].get('png_filter_encoder') else '1 original_arguments_results=1')]
                if m['exception_observer'].get('png_filter_encoder'):
                    p=m['exception_observer']['png_encoder_prior_proof']
                    common+=['SWEET PNG ENCODER installed pid=[0-9]+ backend=ImageIO_UP filter=32 source_sha='+p['encoder_source_sha256']+' proof_token='+p['reference_token']+' fallbacks_original=1']
                if m['exception_observer'].get('postdraw_png_input_metadata'):
                    common+=['SWEET POSTDRAW PNG_CONFIG pid=[0-9]+ revision=1 getters_only=1 pixels_read=0 profile_read=0']
                if m['exception_observer'].get('snapshot_method_observer'):
                    common+=['SWEET SNAPSHOT observer pid=[0-9]+ installed=[1-8] expected=8 limit=64 metadata_only=1 original_arguments_results=1']
                if m['exception_observer'].get('original_request_observer'):
                    common+=['SWEET METAL REQUEST CONFIG pid=[0-9]+ installed=13 expected=13 limit=64 revision=2 original_behavior=1 no_extra_GPU_work=1' if m['exception_observer']['request_observer_revision']==2 else 'SWEET METAL REQUEST CONFIG pid=[0-9]+ installed=6 expected=6 limit=64 arguments_returns_errors_errno=1 no_GPU_work=1']
                if m['exception_observer'].get('quad_route_observer'):
                    q=m['exception_observer'];common+=[f"SWEET QUAD ROUTE CONFIG pid=[0-9]+ installed={q['quad_route_methods']} expected={q['quad_route_methods']} per_api=8 max={q['quad_route_max_records']} revision={q['quad_route_revision']} original_behavior=1 no_extra_GPU_work=1"]
                if 'runtime_Metal_ABI_methods' in m['exception_observer']:
                    common+=['SWEET METAL ABI begin class=_MTLDevice present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1',
                        'SWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0']
                if m['exception_observer'].get('metal_resource_probe'):
                    common+=['SWEET METAL RESOURCE END result=0 buffers=3 bytes=33201 span=65536 final_allocated=0 gpu_submit=0']
                if m['exception_observer'].get('metal_queue_probe'):
                    common+=['SWEET METAL QUEUE END result=0 final_allocated=0 copy_chunks=2 fill_ops=1 copy_ops=2 native_queue=1']
                if m['exception_observer'].get('AIR_compute_probe'):
                    common+=['SWEET AIR COMPUTE END result=0 phases=2 kernel_invocations=128 checked_words=160 guard_checks=32 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('AIR_render_probe'):
                    common+=['SWEET AIR RENDER END result=0 phases=4 draw_ops=3 clear_ops=1 checked_target_bytes=15104 checked_sample_bytes=2048 guard_bytes=6560 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('AIR_constants_probe'):
                    common+=['SWEET AIR CONSTANTS END result=0 phases=3 invocations=192 checked_words=240 guard_checks=48 final_allocated=0 standard_Metal=1']
                if m['exception_observer'].get('original_Metal_registry_probe'):
                    common+=['SWEET METAL DISCOVERY END result=0 same_original_default=1 original_API=1 GPU_work_preverified=1']
                if m['exception_observer'].get('registration_only'):
                    prior=validate_native(m['exception_observer']['prior_native_hardware_proof'],m['metal_resources'])
                    assert prior['version']==version and prior['kernel_sha256']==m['kernel_sha256']
                    common+=['SWEET METAL REGISTRY proof_sha='+native_digest(prior)+' long_probes_in_constructor=0']
                if m['exception_observer'].get('original_pipeline_observer'):
                    common+=['SWEET METAL PIPELINE observer installed=1 methods=3 metadata_only=1 original_returns_errors=1',
                        'SWEET METAL PIPELINE END call=1 returned_nil=[01] error_pointer=[01] error_code=-?[0-9]+ original_return=1']
                if m['exception_observer'].get('AIR_XR_probe'):
                    common+=['SWEET AIR XR END result=0 formats=4 phases=12 checked_raw_output_bytes=34944 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('Metal_IOSurface_probe'):
                    common+=['SWEET METAL SURFACE END result=0 cases=2 phases=6 checked_bytes=14976 row_padding=6480 final_allocated=0 original_surface_GPU=1']
                if m['exception_observer'].get('Metal_mipmap_probe'):
                    common+=['SWEET METAL MIPS END result=0 formats=4 levels=6 generations=8 samples=24 checked_bytes=917504 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_narrow_probe'):
                    common+=['SWEET AIR NARROW END result=0 phases=2 checked_bytes=98304 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_half_probe'):
                    common+=['SWEET AIR HALF END result=0 checked_bytes=131072 phases=2 graphics=1 final_allocated=0 GPU_only=1']
                if m['exception_observer'].get('AIR_MRT_probe'):
                    common+=['SWEET METAL MRT END result=0 XR_formats=4 phases=16 eight_targets=8 ordered_formats=4 checked_bytes=1245184 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('AIR_static_half_probe'):
        common+=['SWEET AIR STATIC HALF END result=0 cases=12 paths=4 checked_bytes=196608 final_allocated=0 original_constants=1 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('Clear_cache_probe'):
        common+=['SWEET CLEAR CACHE END result=0 phases=2 targets=8 checked_bytes=262144 final_allocated=0 original_clear=1 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('IR3_cache_probe'):
        common+=['SWEET IR3 CACHE PROBE END result=0 phases=3 checked_bytes=960 final_allocated=0 cold_and_restore=1 original_pages=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('depth32_probe'):
        common+=['SWEET DEPTH32 END result=0 phases=29 checked_bytes=74240 guard_bytes=38744 read_bytes=26100 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('stencil8_probe'):
        common+=['SWEET STENCIL8 END result=0 phases=99 checked_bytes=329472 guard_bytes=193149 read_bytes=118800 final_allocated=0 GPU_only=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('memoryless_probe'):
        common+=['SWEET MEMORYLESS END result=0 phases=8 checked_bytes=19456 guard_bytes=10888 final_allocated=0 GPU_only=1 transient_pages=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('multisample_probe'):
        common+=['SWEET MULTISAMPLE END result=0 phases=18 checked_bytes=151488 guard_bytes=55098 final_allocated=0 GPU_only=1 original_samples=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('presentation_probe'):
        common+=['SWEET PRESENT PROBE END result=0 phases=5 checked_pixels=765 final_allocated=0 original_drawables=1 display_completion_unverified=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('program_request_export'):
        common+=['SWEET PROGRAM REQUEST EXPORT END result=0 records=2 bytes=[0-9]+ limit=262144 read_only=1 no_compile=1']
    if mode=='gpu' and m.get('exception_observer',{}).get('cif10_PNG_reference'):
        ref=m['exception_observer'];cases=ref['cif10_reference_cases']
        common+=['SWEET CIF10 REFERENCE END result=0 cases='+str(cases)+' original_API=1 author_generated=1 original_UI_pixels_read=0 converter_installed=0']
        if m['exception_observer']['cif10_reference_revision']==2:
            common+=['SWEET CIF10 CONFIG revision=2 PNG_exports=1 vImage_modes=2 original_options=1 converter_installed=0']
        if ref['cif10_reference_revision']==3:
            common+=['SWEET CIF10 CONFIG revision=3 PNG_exports=1 vImage_modes=2 filter_policies=3 original_options=1 converter_installed=0']
    if mode=='gpu' and m.get('exception_observer',{}).get('program_cache_scope'):
        scope=m['exception_observer']['program_cache_scope']
        common+=['SWEET PROGRAM CACHE SCOPE identity='+scope['identity'],
                 'SWEET PROGRAM CACHE SCOPE result=0 inherited=[01] directory='+re.escape(scope['directory'])+' mode=1777 old_files_preserved=1']
    plan={'target_serial':'BH905ND69G','pon_route':6,'max_cycles':1,'budget_seconds':600 if screen_evidence else 480,'camera_required':mode=='gpu' and not screen_evidence,
          'steps':[{'artifact_id':m['id'],'observe_seconds':observe,'stable_seconds':30,'return_seconds':120,'goals':common,
                    'return_evidence':'final-drain','on_pass':'stop','on_failure':'stop'}]}
    if screen_evidence:plan.update(screen_required=True,screen_timeout_seconds=100)
    if mode=='gpu' and screen_evidence:plan['screen_capture_after_seconds']=capture_after
    if grid_control:plan['native_grid_control']=grid_control
    (output/'plan.json').write_text(json.dumps(plan,indent=2)+'\n')
    (output/'package-proof.json').write_text(json.dumps({'baseline':selected,'baseline_sha256':BASE_HASHES[selected],
       'baseline_payload_exact':True,'kernel_sha256':m['kernel_sha256'],'image_sha256':m['sha256'],'cmdline_bytes':len(cmd),
       'blob_count':len(files),'device_operations':0},indent=2)+'\n')
    return m
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--mode',choices=['private','owner','ready','ramcpu','gpu'],required=True)
    p.add_argument('--kernel',required=True);p.add_argument('--baselines',default=str(ROOT/'assets/baselines'));p.add_argument('--output',required=True)
    p.add_argument('--version',type=int,choices=[725,726,727,728,729,730],default=725)
    p.add_argument('--ram-package')
    p.add_argument('--owner-interval',type=int,choices=[90])
    p.add_argument('--screen-evidence',action='store_true')
    p.add_argument('--screen-after',type=int,choices=SCREEN_CAPTURE_TIMES,help='GPU single-frame observation time; default 45 seconds')
    p.add_argument('--thread-sample',action='store_true')
    p.add_argument('--idle-sample',action='store_true',help='Single original Aegir sample after a returned library at observation 180 seconds')
    p.add_argument('--swipe-once',action='store_true',help='One upward swipe after observed F12/LIVE at 120 seconds, then one 150-second scanout')
    p.add_argument('--display-profile',choices=['xz1c-native-2x'],help='Exact four-property native ADT plus existing native1/sf2x0 framebuffer flags')
    p.add_argument('--native-grid-mode',choices=['original','fitted'],help='Explicit original six-row or fitted native grid, using the exact revision3 helper')
    a=p.parse_args();out=Path(a.output).resolve();assert out.is_relative_to(ROOT/'build') and not out.exists();out.mkdir(parents=True)
    m=prepare(a.mode,Path(a.kernel),Path(a.baselines),out,a.version,a.ram_package,a.owner_interval,a.screen_evidence,a.thread_sample,a.screen_after,a.idle_sample,a.swipe_once,a.display_profile,a.native_grid_mode)
    print(json.dumps({k:m[k] for k in ('trial_mode','token','kernel_sha256','sha256','cmdline_bytes')}))
