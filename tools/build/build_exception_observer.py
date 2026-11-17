"""Build a separate observer and exercise the original exception chain natively."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'tools'));from native_proof import validate as validate_native,header as native_header
from program_request import specification as request_specification,header as request_header
from program_cache_scope import specification as cache_scope_specification,header as cache_scope_header
from png_encoder_proof import validate as validate_png,header as png_header
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--metal-resources');p.add_argument('--air-fixture');p.add_argument('--render-fixture');p.add_argument('--constants-fixture');p.add_argument('--register-device',action='store_true');p.add_argument('--xr-probe',action='store_true');p.add_argument('--surface-probe',action='store_true');p.add_argument('--mip-probe',action='store_true');p.add_argument('--target-process',choices=['AegirPoster','backboardd'],default='AegirPoster');p.add_argument('--gles-ram-package');p.add_argument('--registration-only',action='store_true');p.add_argument('--native-proof');p.add_argument('--pipeline-observe',action='store_true');p.add_argument('--narrow-fixture');p.add_argument('--half-fixture');p.add_argument('--mrt-fixture');p.add_argument('--sampler-fixture');p.add_argument('--program-cache-prime',action='store_true');p.add_argument('--static-half-fixture');p.add_argument('--program-cache-export',action='store_true')
p.add_argument('--postdraw-observe',action='store_true');p.add_argument('--snapshot-observe',action='store_true');p.add_argument('--cif10-png-reference',action='store_true');p.add_argument('--png-filter-encoder',action='store_true');p.add_argument('--png-proof');a=p.parse_args()
if a.pipeline_observe:assert a.registration_only and a.target_process=='AegirPoster'
if a.postdraw_observe:assert a.registration_only and a.pipeline_observe and a.target_process=='AegirPoster'
if a.snapshot_observe:assert a.postdraw_observe
if a.cif10_png_reference:assert a.target_process=='backboardd' and a.static_half_fixture and not a.registration_only and not a.program_cache_prime and not a.program_cache_export
if a.png_filter_encoder:assert a.registration_only and a.postdraw_observe and a.png_proof and not a.cif10_png_reference
else:assert not a.png_proof
if a.program_cache_prime:assert a.target_process=='backboardd' and a.sampler_fixture and not a.registration_only
if a.program_cache_export:assert a.target_process=='backboardd' and a.static_half_fixture and not a.registration_only and not a.program_cache_prime
if a.registration_only:
    assert a.target_process=='AegirPoster' and a.metal_resources and a.register_device and a.native_proof
    assert not any((a.air_fixture,a.render_fixture,a.constants_fixture,a.xr_probe,a.surface_probe,a.mip_probe,a.narrow_fixture,a.half_fixture,a.mrt_fixture,a.sampler_fixture,a.static_half_fixture)),'Short registration must not rerun long probes'
else:assert not a.native_proof,'Native proof is only for short registration'
if a.target_process=='backboardd':
    assert a.metal_resources and a.air_fixture and a.render_fixture and a.constants_fixture and a.xr_probe and a.surface_probe and a.mip_probe and a.gles_ram_package and not a.register_device,'Background target requires the complete native suite and its exact GLES package, and cannot register Aegir'
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/'gpu/diagnostics/exception_observe.m';before=source.read_bytes()
(out/'source.m').write_bytes(before)
headers=['gpu/diagnostics/MetalRuntimeObserve.h','gpu/diagnostics/MetalProbeStartup.h']
if a.target_process=='backboardd':headers+=['gpu/sweetgl/sweetgl.h']
if a.pipeline_observe:headers+=['gpu/diagnostics/MetalPipelineObserve.h']
if a.postdraw_observe:headers+=['gpu/diagnostics/PostDrawObserve.h']
if a.snapshot_observe:headers+=['gpu/diagnostics/SnapshotMethodObserve.h']
if a.png_filter_encoder:headers+=['gpu/compat/PNGImageIOEncoder.h','gpu/compat/PNGImageDispatch.h','tools/png_encoder_proof.py']
if a.cif10_png_reference:
    headers+=['gpu/diagnostics/CIF10PNGReferenceProbe.h','gpu/diagnostics/CIF10VImageCompare.h','gpu/diagnostics/CIF10PNGFilterCompare.h','gpu/compat/PNGImageIOEncoder.h']
    png_source_sha=hashlib.sha256((root/'gpu/compat/PNGImageIOEncoder.h').read_bytes()).hexdigest()
if a.program_cache_export:headers+=['gpu/diagnostics/ProgramRequestExport.h','gpu/sweetgl/ProgramCacheCodec.h','gpu/sweetgl/MetalShaderAPI.h','tools/program_request.py']
resource_manifest=None
cache_scope=None
fixture_manifest=None
render_fixture_manifest=None
constants_fixture_manifest=None
narrow_fixture_manifest=None
half_fixture_manifest=None
mrt_fixture_manifest=None
sampler_fixture_manifest=None
static_half_fixture_manifest=None
native_proof=None
if a.metal_resources:
    resources=Path(a.metal_resources).resolve();assert resources.is_relative_to(root/'build')
    resource_manifest=json.loads((resources/'manifest.json').read_bytes())
    assert resource_manifest['only_explicit_resource_factory'] and resource_manifest['system_Metal_plugin_not_advertised']
    assert hashlib.sha256((resources/'MetalResources.dylib').read_bytes()).hexdigest()==resource_manifest['library_sha256']
    if resource_manifest.get('requires_source_scoped_cache'):
        cache_scope=cache_scope_specification(resource_manifest)
        scope_header=cache_scope_header(cache_scope)
        (out/'ProgramCacheScopeIdentity.h').write_bytes(scope_header)
        cache_scope['identity_header_sha256']=hashlib.sha256(scope_header).hexdigest()
        headers+=['gpu/diagnostics/ProgramCacheScope.h','tools/program_cache_scope.py']
    headers+=['gpu/metal/MetalResources.h']
    if not a.registration_only:
        headers+=['gpu/diagnostics/MetalResourceProbe.h']
        if resource_manifest.get('native_blit_command_queue'):headers+=['gpu/diagnostics/MetalQueueProbe.h']
if a.registration_only:
    proof_file=Path(a.native_proof).resolve();assert proof_file.is_relative_to(root/'build')
    native_proof=validate_native(json.loads(proof_file.read_bytes()),resource_manifest)
    (out/'NativeProof.h').write_bytes(native_header(native_proof))
    if a.png_filter_encoder:
        png_file=Path(a.png_proof).resolve();assert png_file.is_relative_to(root/'build')
        png_source_sha=hashlib.sha256((root/'gpu/compat/PNGImageIOEncoder.h').read_bytes()).hexdigest()
        png_proof=validate_png(json.loads(png_file.read_bytes()),png_source_sha,resource_manifest['gles_library_sha256'],resource_manifest['library_sha256'],native_proof['kernel_sha256'])
        (out/'PNGEncoderProof.h').write_bytes(png_header(png_proof))
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
if a.sampler_fixture:
    assert a.target_process=='backboardd' and a.mrt_fixture and resource_manifest.get('native_AIR_constant_samplers')
    fixture=Path(a.sampler_fixture).resolve();assert fixture.is_relative_to(root/'build')
    sampler_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert sampler_fixture_manifest['authored_semantic_fixture'] and sampler_fixture_manifest['fixture']=='sampler'
    assert hashlib.sha256((root/sampler_fixture_manifest['source']).read_bytes()).hexdigest()==sampler_fixture_manifest['source_sha256']
    assert [row['name'] for row in sampler_fixture_manifest['functions']]==['sampler_position','sampler_original','sampler_pair','sampler_lod']
    assert hashlib.sha256((fixture/'AIRSamplerFixture.h').read_bytes()).hexdigest()==sampler_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'sampler.metallib').read_bytes()).hexdigest()==sampler_fixture_manifest['library_sha256']
    (out/'AIRSamplerFixture.h').write_bytes((fixture/'AIRSamplerFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalSamplerProbe.h']
if a.static_half_fixture:
    assert a.target_process=='backboardd' and a.sampler_fixture and resource_manifest.get('native_static_half_bits')
    fixture=Path(a.static_half_fixture).resolve();assert fixture.is_relative_to(root/'build')
    static_half_fixture_manifest=json.loads((fixture/'manifest.json').read_bytes())
    assert static_half_fixture_manifest['authored_semantic_fixture'] and static_half_fixture_manifest['fixture']=='static-half'
    assert hashlib.sha256((root/static_half_fixture_manifest['source']).read_bytes()).hexdigest()==static_half_fixture_manifest['source_sha256']
    assert [row['name'] for row in static_half_fixture_manifest['functions']]==['static_half']
    assert hashlib.sha256((fixture/'AIRStaticHalfFixture.h').read_bytes()).hexdigest()==static_half_fixture_manifest['header_sha256']
    assert hashlib.sha256((fixture/'static-half.metallib').read_bytes()).hexdigest()==static_half_fixture_manifest['library_sha256']
    (out/'AIRStaticHalfFixture.h').write_bytes((fixture/'AIRStaticHalfFixture.h').read_bytes())
    headers+=['gpu/diagnostics/MetalStaticHalfProbe.h']
if resource_manifest and resource_manifest.get('native_static_half_bits') and not a.registration_only:
    assert static_half_fixture_manifest,'New static-half library needs its own complete hardware probe'
headers=list(dict.fromkeys(headers))
request_export=None
if a.program_cache_export:
    export_ram=json.loads((Path(a.gles_ram_package)/'manifest.json').read_bytes())
    request_export=request_specification(export_ram,framing_revision=2)
    request_identity=request_header(request_export)
    (out/'ProgramRequestIdentity.h').write_bytes(request_identity)
    request_export['identity_header_sha256']=hashlib.sha256(request_identity).hexdigest()
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
if cache_scope:
    run(['clang','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_program_cache_scope.c','-o',out/'test_cache_scope'],'cache-scope-host-compile')
    assert 'PASS' in run([out/'test_cache_scope'],'cache-scope-host-test')
if a.program_cache_export:
    run(['clang','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_program_request_export.c','-o',out/'test_request_export'],'request-export-host-compile')
    assert 'PASS' in run([out/'test_request_export'],'request-export-host-test')
if a.pipeline_observe:
    run(['clang','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_metal_pipeline_observe.m','-framework','Foundation','-framework','Metal','-o',out/'test_pipeline_observer'],'pipeline-host-compile')
    assert 'PASS' in run([out/'test_pipeline_observer'],'pipeline-host-test')
if a.cif10_png_reference:
    run(['clang','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_cif10_png_reference.m',
        '-framework','Foundation','-framework','CoreGraphics','-framework','ImageIO','-framework','Accelerate','-o',out/'test_cif10_png_reference'],'cif10-reference-host-compile')
    assert 'CIF10 PNG reference PASS:' in run([out/'test_cif10_png_reference'],'cif10-reference-host-test')
if a.png_filter_encoder:
    run(['clang','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_png_image_dispatch.m',
        '-framework','Foundation','-framework','CoreGraphics','-framework','ImageIO','-o',out/'test_png_image_dispatch'],'png-dispatch-host-compile')
    assert 'PNG image dispatch PASS:' in run([out/'test_png_image_dispatch'],'png-dispatch-host-test')
if a.postdraw_observe:
    run(['clang','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_postdraw_observe.m',
        '-framework','Foundation','-framework','CoreGraphics','-framework','ImageIO','-framework','ColorSync',
        '-o',out/'test_postdraw_observer'],'postdraw-host-compile')
    postdraw_test=run([out/'test_postdraw_observer'],'postdraw-host-test')
    assert 'postdraw observer PASS:' in postdraw_test
    assert postdraw_test.count('SWEET POSTDRAW BEGIN ')==64 and postdraw_test.count('SWEET POSTDRAW END ')==64
    run(['clang','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_png_input_observe.m',
        '-framework','Foundation','-framework','CoreGraphics','-framework','ImageIO','-framework','ColorSync',
        '-o',out/'test_png_input_observer'],'png-input-host-compile')
    assert 'PNG input observer PASS:' in run([out/'test_png_input_observer'],'png-input-host-test')
if a.snapshot_observe:
    run(['clang','-fobjc-exceptions','-O1','-g','-fsanitize=address,undefined',root/'tools/build/test_snapshot_method_observe.m',
        '-framework','Foundation','-framework','Metal','-framework','CoreGraphics','-framework','ImageIO','-framework','ColorSync',
        '-o',out/'test_snapshot_method_observer'],'snapshot-method-host-compile')
    snapshot_test=run([out/'test_snapshot_method_observer'],'snapshot-method-host-test')
    assert 'snapshot method observer PASS:' in snapshot_test
    assert snapshot_test.count('SWEET POSTDRAW BEGIN ')==64 and snapshot_test.count('SWEET POSTDRAW END ')==64
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
if a.postdraw_observe:extra+=['-DSWEET_POSTDRAW_OBSERVE=1','-framework','UIKit','-framework','CoreGraphics','-framework','ImageIO','-framework','ColorSync']
if a.snapshot_observe:extra+=['-DSWEET_SNAPSHOT_METHOD_OBSERVE=1']
if a.png_filter_encoder:extra+=['-DSWEET_PNG_FILTER_ENCODER=1','-I',out]
if a.cif10_png_reference:extra+=['-DSWEET_CIF10_PNG_REFERENCE=1','-DSWEET_PNG_IMAGEIO_SOURCE_SHA="'+png_source_sha+'"','-framework','UIKit','-framework','CoreGraphics','-framework','ImageIO','-framework','Accelerate']
if a.xr_probe:extra+=['-DSWEET_AIR_XR_PROBE=1']
if a.surface_probe:extra+=['-DSWEET_METAL_SURFACE_PROBE=1','-framework','IOSurface']
if a.mip_probe:extra+=['-DSWEET_METAL_MIP_PROBE=1']
if narrow_fixture_manifest:extra+=['-DSWEET_AIR_NARROW_PROBE=1','-I',out]
if half_fixture_manifest:extra+=['-DSWEET_AIR_HALF_PROBE=1','-I',out]
if mrt_fixture_manifest:extra+=['-DSWEET_AIR_MRT_PROBE=1','-I',out]
if sampler_fixture_manifest:extra+=['-DSWEET_AIR_SAMPLER_PROBE=1','-I',out]
if sampler_fixture_manifest and resource_manifest.get('native_program_binary_cache'):extra+=['-DSWEET_PROGRAM_CACHE_PROBE=1']
if static_half_fixture_manifest:extra+=['-DSWEET_AIR_STATIC_HALF_PROBE=1','-I',out]
if a.program_cache_prime:extra+=['-DSWEET_PROGRAM_CACHE_PRIME=1']
if a.program_cache_export:extra+=['-DSWEET_PROGRAM_CACHE_EXPORT=1','-I',out]
if cache_scope:extra+=['-DSWEET_PROGRAM_CACHE_SCOPE=1','-I',out]
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
if a.cif10_png_reference:manifest.update(cif10_PNG_reference=True,cif10_reference_revision=3,cif10_reference_cases=6,
    cif10_reference_author_pixels_only=True,cif10_reference_original_UIImage_API=True,cif10_reference_converter_not_installed=True,
    cif10_reference_host_fixture_bounds_nil_throw_input_passed=True,cif10_reference_PNG_exports=True,
    cif10_reference_vImage_modes=2,cif10_reference_vImage_guard_buffer_host_passed=True,
    cif10_reference_filter_policies=3,cif10_reference_lossless_filter_compare=True,cif10_reference_candidate_differences_telemetry=True,
    png_imageio_encoder_source_sha256=png_source_sha,cif10_reference_shared_encoder=True,
    cif10_reference_host_test_source_sha256=hashlib.sha256((root/'tools/build/test_cif10_png_reference.m').read_bytes()).hexdigest())
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
if sampler_fixture_manifest:manifest.update(AIR_sampler_probe=True,AIR_sampler_fixture=sampler_fixture_manifest)
if sampler_fixture_manifest and resource_manifest.get('native_program_binary_cache'):manifest['program_cache_probe']=True
if static_half_fixture_manifest:manifest.update(AIR_static_half_probe=True,AIR_static_half_fixture=static_half_fixture_manifest)
if a.program_cache_prime:manifest['program_cache_prime']=True
if request_export:manifest['program_request_export']=request_export
if cache_scope:manifest['program_cache_scope']=cache_scope
if a.registration_only:manifest.update(registration_only=True,prior_native_hardware_proof=native_proof,long_native_probes_not_in_constructor=True,
    native_proof_header_sha256=hashlib.sha256((out/'NativeProof.h').read_bytes()).hexdigest())
if a.pipeline_observe:manifest.update(original_pipeline_observer=True,actual_factory_return_error_pointer_exception_forwarding_host_passed=True)
if a.postdraw_observe:manifest.update(original_postdraw_observer=True,postdraw_observer_revision=3 if a.png_filter_encoder else 2,postdraw_observations_max=64,
    postdraw_apis=['png','add_image','finalize','colorsync'],postdraw_return_nil_throw_errno_nesting_host_passed=True,
    postdraw_real_PNG_bytes_pixels_ICC_input_host_passed=True,postdraw_host_test_source_sha256=hashlib.sha256((root/'tools/build/test_postdraw_observe.m').read_bytes()).hexdigest(),
    postdraw_original_pixel_and_profile_options_preserved=True,postdraw_durations_inclusive_not_additive=True,
    postdraw_png_input_metadata=True,png_input_getters_exceptions_errno_host_passed=True,
    png_input_no_pixel_or_profile_reads_host_passed=True,
    png_input_host_test_source_sha256=hashlib.sha256((root/'tools/build/test_png_input_observe.m').read_bytes()).hexdigest())
if a.png_filter_encoder:manifest.update(png_filter_encoder=True,png_imageio_encoder_source_sha256=png_source_sha,
    png_encoder_prior_proof=png_proof,png_encoder_proof_header_sha256=hashlib.sha256(png_header(png_proof)).hexdigest(),
    png_encoder_fallback_nil_throw_errno_input_metadata_host_passed=True,png_encoder_preserves_pixels_profile_metadata=True,
    png_encoder_changes_only_encoding_path_and_compression=True,
    png_encoder_host_test_source_sha256=hashlib.sha256((root/'tools/build/test_png_image_dispatch.m').read_bytes()).hexdigest())
if a.snapshot_observe:manifest.update(snapshot_method_observer=True,snapshot_method_observer_revision=1,snapshot_method_count_max=8,
    snapshot_method_apis=['pipeline','library_url','library_bundle','snapshot','wait','execute','read','read_full'],
    snapshot_method_exact_ABI_arguments_returns_errors_errno_throws_host_passed=True,
    snapshot_method_read_bytes_guards_and_ABI_owner_repeat_rejection_host_passed=True,
    snapshot_method_target_coverage_requires_actual_metadata=True,
    snapshot_method_host_test_source_sha256=hashlib.sha256((root/'tools/build/test_snapshot_method_observe.m').read_bytes()).hexdigest())
assert all(hashlib.sha256((root/name).read_bytes()).hexdigest()==digest for name,digest in source_files.items())
(out/'manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
