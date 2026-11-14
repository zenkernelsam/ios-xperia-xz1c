"""Build a separate observer and exercise the original exception chain natively."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--metal-resources');p.add_argument('--air-fixture');p.add_argument('--render-fixture');p.add_argument('--constants-fixture');p.add_argument('--register-device',action='store_true');p.add_argument('--xr-probe',action='store_true');p.add_argument('--surface-probe',action='store_true');a=p.parse_args()
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
source=root/'gpu/diagnostics/exception_observe.m';before=source.read_bytes()
(out/'source.m').write_bytes(before)
headers=['gpu/diagnostics/MetalRuntimeObserve.h']
resource_manifest=None
fixture_manifest=None
render_fixture_manifest=None
constants_fixture_manifest=None
if a.metal_resources:
    resources=Path(a.metal_resources).resolve();assert resources.is_relative_to(root/'build')
    resource_manifest=json.loads((resources/'manifest.json').read_bytes())
    assert resource_manifest['only_explicit_resource_factory'] and resource_manifest['system_Metal_plugin_not_advertised']
    assert hashlib.sha256((resources/'MetalResources.dylib').read_bytes()).hexdigest()==resource_manifest['library_sha256']
    headers+=['gpu/diagnostics/MetalResourceProbe.h','gpu/metal/MetalResources.h']
    if resource_manifest.get('native_blit_command_queue'):headers+=['gpu/diagnostics/MetalQueueProbe.h']
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
    assert resource_manifest and render_fixture_manifest and constants_fixture_manifest
    headers+=['gpu/diagnostics/MetalDiscoveryProbe.h']
if a.xr_probe:
    assert resource_manifest and render_fixture_manifest
    headers+=['gpu/diagnostics/MetalXRProbe.h','gpu/sweetgl/XRColourTables.h']
if a.surface_probe:
    assert resource_manifest and render_fixture_manifest
    headers+=['gpu/diagnostics/MetalSurfaceProbe.h','gpu/sweetgl/MetalBackingAPI.h']
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
library=out/'exception_observe.dylib'
extra=['-DSWEET_METAL_RESOURCE_PROBE=1',resources/'MetalResources.dylib'] if resource_manifest else []
if resource_manifest and resource_manifest.get('native_blit_command_queue'):extra+=['-DSWEET_METAL_QUEUE_PROBE=1']
if fixture_manifest:extra+=['-DSWEET_AIR_COMPUTE_PROBE=1','-I',out]
if render_fixture_manifest:extra+=['-DSWEET_AIR_RENDER_PROBE=1','-I',out,'-framework','Metal']
if constants_fixture_manifest:extra+=['-DSWEET_AIR_CONSTANTS_PROBE=1','-I',out,'-framework','Metal']
if a.register_device:extra+=['-DSWEET_METAL_DISCOVERY_PROBE=1']
if a.xr_probe:extra+=['-DSWEET_AIR_XR_PROBE=1']
if a.surface_probe:extra+=['-DSWEET_METAL_SURFACE_PROBE=1','-framework','IOSurface']
run(['xcrun','--sdk','iphoneos','clang','-arch','arm64','-miphoneos-version-min=16.0',
    '-fobjc-exceptions','-O2','-dynamiclib',source,'-framework','Foundation',
    '-Wl,-not_for_dyld_shared_cache','-install_name','/dev/sglram/e','-o',library,*extra],'ios-compile')
run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'verify-signature')
assert source.read_bytes()==before
manifest=dict(source='gpu/diagnostics/exception_observe.m',source_sha256=hashlib.sha256(before).hexdigest(),
    library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),library_bytes=library.stat().st_size,
    native_original_chain_and_bound_test_passed=True,host_asan_ubsan_passed=True,
    original_exception_behavior_preserved=True,target_process='AegirPoster',observations_max=16,device_operations=0)
manifest.update(renderer_abi_and_device_observer_test_passed=True,renderer_observations_max=32,
    metadata_only_no_buffer_allocation=True,original_renderer_return_and_exception_preserved=True)
manifest.update(source_files=source_files,runtime_Metal_ABI_methods=22,metadata_observer_no_device_calls_test_passed=True)
if resource_manifest:manifest.update(metal_resource_probe=True,metal_resources=resource_manifest,
    metadata_and_resource_probe_distinct=True,resource_probe_does_not_submit_GPU_commands=True)
if resource_manifest and resource_manifest.get('native_blit_command_queue'):
    manifest.update(metal_queue_probe=True,queue_probe_submits_actual_GPU_commands=True)
if fixture_manifest:manifest.update(AIR_compute_probe=True,AIR_fixture=fixture_manifest)
if render_fixture_manifest:manifest.update(AIR_render_probe=True,AIR_render_fixture=render_fixture_manifest)
if constants_fixture_manifest:manifest.update(AIR_constants_probe=True,AIR_constants_fixture=constants_fixture_manifest)
if a.register_device:manifest.update(original_Metal_registry_probe=True,registry_probe_process='AegirPoster',registry_functions_not_replaced=True)
if a.xr_probe:manifest['AIR_XR_probe']=True
if a.surface_probe:manifest['Metal_IOSurface_probe']=True
assert all(hashlib.sha256((root/name).read_bytes()).hexdigest()==digest for name,digest in source_files.items())
(out/'manifest.json').write_bytes((json.dumps(manifest,indent=2)+'\n').encode())
print(json.dumps(manifest,indent=2))
