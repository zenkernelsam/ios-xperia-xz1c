"""Build a separate observer and exercise the original exception chain natively."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'));from native_proof import validate as validate_native,header as native_header
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--metal-resources');p.add_argument('--air-fixture');p.add_argument('--render-fixture');p.add_argument('--constants-fixture');p.add_argument('--register-device',action='store_true');p.add_argument('--xr-probe',action='store_true');p.add_argument('--surface-probe',action='store_true');p.add_argument('--mip-probe',action='store_true');p.add_argument('--target-process',choices=['AegirPoster','backboardd'],default='AegirPoster');p.add_argument('--gles-ram-package');p.add_argument('--registration-only',action='store_true');p.add_argument('--native-proof');p.add_argument('--pipeline-observe',action='store_true');p.add_argument('--narrow-fixture');p.add_argument('--half-fixture');p.add_argument('--mrt-fixture');a=p.parse_args()
if a.pipeline_observe:assert a.registration_only and a.target_process=='AegirPoster'
if a.registration_only:
    assert a.target_process=='AegirPoster' and a.metal_resources and a.register_device and a.native_proof
    assert not any((a.air_fixture,a.render_fixture,a.constants_fixture,a.xr_probe,a.surface_probe,a.mip_probe,a.narrow_fixture,a.half_fixture,a.mrt_fixture)),'Short registration must not rerun long probes'
else:assert not a.native_proof,'Native proof is only for short registration'
if a.target_process=='backboardd':
    assert a.metal_resources and a.air_fixture and a.render_fixture and a.constants_fixture and a.xr_probe and a.surface_probe and a.mip_probe and a.gles_ram_package and not a.register_device,'Background target requires the complete native suite and its exact GLES package, and cannot register Aegir'
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/'gpu/diagnostics/exception_observe.m';before=source.read_bytes()
(out/'source.m').write_bytes(before)
headers=['gpu/diagnostics/MetalRuntimeObserve.h','gpu/diagnostics/MetalProbeStartup.h']
if a.target_process=='backboardd':headers+=['gpu/sweetgl/sweetgl.h']
if a.pipeline_observe:headers+=['gpu/diagnostics/MetalPipelineObserve.h']
resource_manifest=None
fixture_manifest=None
render_fixture_manifest=None
constants_fixture_manifest=None
narrow_fixture_manifest=None
half_fixture_manifest=None
mrt_fixture_manifest=None
native_proof=None
if a.metal_resources:
    resources=Path(a.metal_resources).resolve();assert resources.is_relative_to(root/'build')
    resource_manifest=json.loads((resources/'manifest.json').read_bytes())
    assert resource_manifest['only_explicit_resource_factory'] and resource_manifest['system_Metal_plugin_not_advertised']
    assert hashlib.sha256((resources/'MetalResources.dylib').read_bytes()).hexdigest()==resource_manifest['library_sha256']
    headers+=['gpu/metal/MetalResources.h']
    if not a.registration_only:
        headers+=['gpu/diagnostics/MetalResourceProbe.h']
        if resource_manifest.get('native_blit_command_queue'):headers+=['gpu/diagnostics/MetalQueueProbe.h']
if a.registration_only:
    proof_file=Path(a.native_proof).resolve();assert proof_file.is_relative_to(root/'build')
    native_proof=validate_native(json.loads(proof_file.read_bytes()),resource_manifest)
    (out/'NativeProof.h').write_bytes(native_header(native_proof))
if a.air_fixture:
    assert resource_manifest and resource_manifest.get('AIR_compute_pipeline')
    fixture=Path(a.air_fixture).resolve();assert fixture.is_relative_to(root/'build')
    fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert fixture_manifest['authored_semantic_fixture']
    assert hashlib.sha256((fixture/'AIRFixture.h').read_bytes()).hexdigest()==fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'basic.metallib').read_bytes()).hexdigest()==fixture_manifest['library_sha256']
    (out/'AIRFixture.h').write_bytes((fixture/'AIRFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalComputeProbe.h']
if a.render_fixture:
    assert resource_manifest and resource_manifest.get('native_texture_render_pipeline')
    fixture=Path(a.render_fixture).resolve();assert fixture.is_relative_to(root/'build')
    render_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert render_fixture_manifest['authored_semantic_fixture'] and render_fixture_manifest['fixture']=='render'
    assert hashlib.sha256((fixture/'AIRRenderFixture.h').read_bytes()).hexdigest()==render_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'render.metallib').read_bytes()).hexdigest()==render_fixture_manifest['library_sha256']
    (out/'AIRRenderFixture.h').write_bytes((fixture/'AIRRenderFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalRenderProbe.h']
if a.constants_fixture:
    assert resource_manifest and resource_manifest.get('native_function_constants')
    fixture=Path(a.constants_fixture).resolve();assert fixture.is_relative_to(root/'build')
    constants_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert constants_fixture_manifest['authored_semantic_fixture'] and constants_fixture_manifest['fixture']=='constants'
    assert hashlib.sha256((fixture/'AIRConstantsFixture.h').read_bytes()).hexdigest()==constants_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'constants.metallib').read_bytes()).hexdigest()==constants_fixture_manifest['library_sha256']
    (out/'AIRConstantsFixture.h').write_bytes((fixture/'AIRConstantsFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalConstantsProbe.h']
if a.register_device:
    assert resource_manifest and (native_proof or (render_fixture_manifest and constants_fixture_manifest))
    headers+=['gpu/diagnostics/MetalDiscoveryProbe.h']
if a.xr_probe:
    assert resource_manifest and render_fixture_manifest
    headers+=['gpu/diagnostics/MetalXRProbe.h','gpu/sweetgl/XRColourTables.h']
if a.surface_probe:
    assert resource_manifest and render_fixture_manifest
    headers+=['gpu/diagnostics/MetalSurfaceProbe.h','gpu/sweetgl/MetalBackingAPI.h']
if a.mip_probe:
    assert resource_manifest and render_fixture_manifest and resource_manifest.get('native_mipmaps')
    headers+=['gpu/diagnostics/MetalMipProbe.h','gpu/sweetgl/MetalShaderAPI.h','gpu/sweetgl/XRColourTables.h']
if a.narrow_fixture:
    assert resource_manifest and resource_manifest['AIR_frontend'].get('local_narrow_conversion_adaptation')
    fixture=Path(a.narrow_fixture).resolve();assert fixture.is_relative_to(root/'build')
    narrow_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert narrow_fixture_manifest['authored_semantic_fixture'] and narrow_fixture_manifest['fixture']=='narrow'
    assert hashlib.sha256((fixture/'AIRNarrowFixture.h').read_bytes()).hexdigest()==narrow_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'narrow.metallib').read_bytes()).hexdigest()==narrow_fixture_manifest['library_sha256']
    (out/'AIRNarrowFixture.h').write_bytes((fixture/'AIRNarrowFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalNarrowProbe.h','gpu/diagnostics/NarrowProbeValues.h']
if a.half_fixture:
    assert resource_manifest and resource_manifest['AIR_frontend'].get('local_half_storage_and_rounding_adaptation') and a.narrow_fixture
    fixture=Path(a.half_fixture).resolve();assert fixture.is_relative_to(root/'build')
    half_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert half_fixture_manifest['authored_semantic_fixture'] and half_fixture_manifest['fixture']=='half'
    assert hashlib.sha256((fixture/'AIRHalfFixture.h').read_bytes()).hexdigest()==half_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'half.metallib').read_bytes()).hexdigest()==half_fixture_manifest['library_sha256']
    (out/'AIRHalfFixture.h').write_bytes((fixture/'AIRHalfFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalHalfProbe.h','gpu/diagnostics/HalfProbeValues.h']
if a.mrt_fixture:
    assert resource_manifest and resource_manifest.get('native_MRT_and_XR_blend') and a.half_fixture
    fixture=Path(a.mrt_fixture).resolve();assert fixture.is_relative_to(root/'build')
    mrt_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert mrt_fixture_manifest['authored_semantic_fixture'] and mrt_fixture_manifest['fixture']=='mrt'
    assert hashlib.sha256((root/mrt_fixture_manifest['source']).read_bytes()).hexdigest()==mrt_fixture_manifest['source_sha256'],'Current MRT fixture source differs from its assembled AIR'
    assert [row['name'] for row in mrt_fixture_manifest['functions']]==['mrt_position','mrt_colour','mrt_changed','mrt_eight','mrt_sample','mrt_order_position','mrt_order_colour']
    assert hashlib.sha256((fixture/'AIRMRTFixture.h').read_bytes()).hexdigest()==mrt_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'mrt.metallib').read_bytes()).hexdigest()==mrt_fixture_manifest['library_sha256']
    (out/'AIRMRTFixture.h').write_bytes((fixture/'AIRMRTFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalMRTProbe.h','gpu/diagnostics/MRTProbeValues.h','gpu/sweetgl/XRColourTables.h']
headers=list(dict.fromkeys(headers))
source_files={str(source.relative_to(root)):hashlib.sha256(before).hexdigest()}
for name in headers:
    data=(root/name).read_bytes();(out/Path(name).name).write_bytes(data)
    source_files[name]=hashlib.sha256(data).hexdigest()
def run(argv,name):
    result=subprocess.run(list(map(str,argv)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode==0,result.stdout+result.stderr
    return result.stdout
run(['clang','-DLILAC_EXCEPTION_TEST=1','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',
     source,'-framework','Foundation','-o',out/'test_observer'],'host-compile')
test=run([out/'test_observer'],'host-test')
assert test.count('SWEET EXCEPTION begin call=')==16 and 'PASS' in test
assert 'reason="line1 line2"' in test
assert test.count('SWEET METAL uniform call=')==32 and 'renderer ABI, nil/device metadata' in test
assert 'device_nil=1 device_class=nil new_buffer_method=0' in test
assert 'device_nil=0 device_class=NUNITestDevice new_buffer_method=1' in test
assert 'class=MetalABIHostFixture present=1 limits_bytes=264 limits_alignment=8 limits_bounded=1' in test
assert 'owner=device_base selector=initWithAcceleratorPort: present=1 types_hex=' in test
assert 'SWEET METAL ABI end methods=22 metadata_only=1 instances_created=0 gpu_operations=0' in test
run(['clang','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_metal_startup.c','-lpthread','-o',out/'test_startup'],'startup-host-compile')
assert 'PASS' in run([out/'test_startup'],'startup-host-test')
if a.pipeline_observe:
    run(['clang','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_metal_pipeline_observe.m','-framework','Foundation','-framework','Metal','-o',out/'test_pipeline_observer'],'pipeline-host-compile')
    assert 'PASS' in run([out/'test_pipeline_observer'],'pipeline-host-test')
library=out/'exception_observe.dylib'
extra=[resources/'MetalResources.dylib'] if resource_manifest else []
if resource_manifest and not a.registration_only:extra+=['-DSWEET_METAL_RESOURCE_PROBE=1']
if resource_manifest and resource_manifest.get('native_blit_command_queue') and not a.registration_only:extra+=['-DSWEET_METAL_QUEUE_PROBE=1']
if a.registration_only:extra+=['-DSWEET_METAL_REGISTRY_ONLY=1','-I',out,'-framework','Metal']
if fixture_manifest:extra+=['-DSWEET_AIR_COMPUTE_PROBE=1','-I',out]
if render_fixture_manifest:extra+=['-DSWEET_AIR_RENDER_PROBE=1','-I',out,'-framework','Metal']
if constants_fixture_manifest:extra+=['-DSWEET_AIR_CONSTANTS_PROBE=1','-I',out,'-framework','Metal']
if a.register_device:extra+=['-DSWEET_METAL_DISCOVERY_PROBE=1']
if a.pipeline_observe:extra+=['-DSWEET_METAL_PIPELINE_OBSERVE=1','-framework','Metal']
if a.xr_probe:extra+=['-DSWEET_AIR_XR_PROBE=1']
if a.surface_probe:extra+=['-DSWEET_METAL_SURFACE_PROBE=1','-framework','IOSurface']
if a.mip_probe:extra+=['-DSWEET_METAL_MIP_PROBE=1']
if narrow_fixture_manifest:extra+=['-DSWEET_AIR_NARROW_PROBE=1','-I',out]
if half_fixture_manifest:extra+=['-DSWEET_AIR_HALF_PROBE=1','-I',out]
if mrt_fixture_manifest:extra+=['-DSWEET_AIR_MRT_PROBE=1','-I',out]
if a.target_process=='backboardd':
    gles=Path(a.gles_ram_package).resolve();assert gles.is_relative_to(root/'build')
    gm=json.loads((gles/'manifest.json').read_bytes())
    assert gm['library_sha256']==resource_manifest['gles_library_sha256']==hashlib.sha256((gles/'sweetgl.dylib').read_bytes()).hexdigest()
    extra+=['-DSWEET_METAL_BACKGROUND_PROBE=1',gles/'sweetgl.dylib']
run(['xcrun','--sdk','iphoneos','clang','-arch','arm64','-miphoneos-version-min=16.0',
    '-fobjc-exceptions','-O2','-dynamiclib',source,'-framework','Foundation',
    '-Wl,-not_for_dyld_shared_cache','-install_name','/dev/sglram/e','-o',library,*extra],'ios-compile')
run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'verify-signature')
assert source.read_bytes()==before
manifest=dict(source='gpu/diagnostics/exception_observe.m',source_sha256=hashlib.sha256(before).hexdigest(),
    library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),library_bytes=library.stat().st_size,
    native_original_chain_and_bound_test_passed=True,host_asan_ubsan_passed=True,
    original_exception_behavior_preserved=True,target_process=a.target_process,observations_max=16,device_operations=0)
if a.target_process=='AegirPoster':manifest.update(renderer_abi_and_device_observer_test_passed=True,renderer_observations_max=32,
    metadata_only_no_buffer_allocation=True,original_renderer_return_and_exception_preserved=True)
manifest.update(source_files=source_files,runtime_Metal_ABI_methods=22,metadata_observer_no_device_calls_test_passed=True)
manifest.update(probe_startup_revision=1,actual_startup_readiness_host_asan_ubsan_passed=True,startup_host_source_sha256=hashlib.sha256((root/'tools/build/test_metal_startup.c').read_bytes()).hexdigest())
if a.target_process=='backboardd':manifest.update(background_full_suite=True,waits_for_same_process_GLES_suite=True,original_Aegir_not_modified_or_registered=True)
if resource_manifest:manifest.update(metal_resources=resource_manifest)
if resource_manifest and not a.registration_only:manifest.update(metal_resource_probe=True,
    metadata_and_resource_probe_distinct=True,resource_probe_does_not_submit_GPU_commands=True)
if resource_manifest and resource_manifest.get('native_blit_command_queue') and not a.registration_only:
    manifest.update(metal_queue_probe=True,queue_probe_submits_actual_GPU_commands=True)
if fixture_manifest:manifest.update(AIR_compute_probe=True,AIR_fixture=fixture_manifest)
if render_fixture_manifest:manifest.update(AIR_render_probe=True,AIR_render_fixture=render_fixture_manifest)
if constants_fixture_manifest:manifest.update(AIR_constants_probe=True,AIR_constants_fixture=constants_fixture_manifest)
if a.register_device:manifest.update(original_Metal_registry_probe=True,registry_probe_process='AegirPoster',registry_functions_not_replaced=True)
if a.xr_probe:manifest['AIR_XR_probe']=True
if a.surface_probe:manifest['Metal_IOSurface_probe']=True
if a.mip_probe:manifest['Metal_mipmap_probe']=True
if narrow_fixture_manifest:manifest.update(AIR_narrow_probe=True,AIR_narrow_fixture=narrow_fixture_manifest)
if half_fixture_manifest:manifest.update(AIR_half_probe=True,AIR_half_fixture=half_fixture_manifest)
if mrt_fixture_manifest:manifest.update(AIR_MRT_probe=True,AIR_MRT_fixture=mrt_fixture_manifest)
if a.registration_only:manifest.update(registration_only=True,prior_native_hardware_proof=native_proof,long_native_probes_not_in_constructor=True,
    native_proof_header_sha256=hashlib.sha256((out/'NativeProof.h').read_bytes()).hexdigest())
if a.pipeline_observe:manifest.update(original_pipeline_observer=True,actual_factory_return_error_pointer_exception_forwarding_host_passed=True)
assert all(hashlib.sha256((root/name).read_bytes()).hexdigest()==digest for name,digest in source_files.items())
(out/'manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
