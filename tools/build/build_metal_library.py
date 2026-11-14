"""Build the explicit native resource factory against its actual signed GLES input."""
import argparse,hashlib,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--ram-package',required=True);parser.add_argument('--output',required=True)
parser.add_argument('--air-package')
args=parser.parse_args();out=Path(args.output).absolute();assert out.is_relative_to(root/'build') and not out.exists();out.mkdir(parents=True)
package=Path(args.ram_package).resolve();manifest=json.loads((package/'manifest.json').read_bytes());gles=package/'sweetgl.dylib'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(gles)==manifest['library_sha256']
files=['gpu/metal/MetalResources.mm','gpu/metal/MetalResources.h','gpu/metal/MetalBufferRules.h',
       'gpu/metal/MetalDeviceLimits16.h','gpu/sweetgl/MetalBackingAPI.h','gpu/sweetgl/MetalBackingRules.h',
       'gpu/sweetgl/MetalBlitPlan.h','gpu/metal/MetalCommandQueue.inc','gpu/metal/MetalQueueState.h']
sources={p:sha(root/p) for p in files}
extra=['-DSWEET_METAL_NO_COMPUTE=1']
air_manifest=None
if args.air_package:
    air=Path(args.air_package).resolve();assert air.is_relative_to(root/'build')
    air_manifest=json.loads((air/'manifest.json').read_bytes())
    assert air_manifest['actual_AIR_stage_translation_verified'] and not air_manifest['llvm_import_audit']['missing_target_imports']
    assert air_manifest['forced_legacy_interface_host_test_passed']
    assert sha(air/'AIRFrontend.ios.o')==air_manifest['compiled_AIR_object_sha256']
    assert all(sha(root/path)==digest for path,digest in air_manifest['source_files'].items())
    files+=['gpu/metal/MetalCompute.inc','gpu/metal/MetalFunctionConstants.inc','gpu/metal/MetalRender.inc','gpu/metal/MetalTextureRules.h',
        'gpu/metal/air/ESSLTarget.h','gpu/metal/air/Compiler.h','gpu/sweetgl/MetalShaderAPI.h','gpu/sweetgl/XRColourShader.h']
    sources={p:sha(root/p) for p in files}
    sources.update(air_manifest['source_files'])
    extra=['-I',root/'gpu/metal/air/reference',air/'AIRFrontend.ios.o',
        Path(subprocess.check_output(['xcrun','--sdk','iphoneos','--show-sdk-path'],text=True).strip())/'usr/lib/libLLVM.tbd']
def run(command,name):
    result=subprocess.run(list(map(str,command)),capture_output=True,text=True)
    (out/(name+'.log')).write_text(result.stdout+result.stderr);assert result.returncode==0,result.stdout+result.stderr
library=out/'MetalResources.dylib'
run(['xcrun','--sdk','iphoneos','clang++','-arch','arm64','-miphoneos-version-min=16.0','-std=c++17','-fno-objc-arc',
    '-O2','-dynamiclib','-Wl,-undefined,dynamic_lookup','-Wl,-not_for_dyld_shared_cache',
    '-install_name','/dev/sglram/MetalResources.dylib',root/'gpu/metal/MetalResources.mm',gles,
    '-framework','Foundation','-framework','Metal','-framework','IOKit','-framework','IOSurface','-o',library,*extra],'ios-link')
run(['codesign','-f','-s','-',library],'codesign');run(['codesign','--verify',library],'verify-signature')
assert all(sha(root/p)==digest for p,digest in sources.items()) and sha(gles)==manifest['library_sha256']
report=dict(source_files=sources,library_file='MetalResources.dylib',library_sha256=sha(library),library_bytes=library.stat().st_size,
    gles_library_sha256=sha(gles),only_explicit_resource_factory=True,system_Metal_plugin_not_advertised=True,
    resource_device_acceptance_pending=True,device_operations=0)
report.update(native_blit_command_queue=True,hardware_queue_blit_acceptance_pending=True,
    system_Metal_shader_rendering_still_pending=True,blit_transport_evidence_revision=1)
if air_manifest:report.update(AIR_compute_pipeline=True,AIR_frontend=air_manifest,actual_AIR_compute_GPU_acceptance_pending=True,
    native_texture_render_pipeline=True,actual_AIR_render_GPU_acceptance_pending=True)
if air_manifest:report.update(native_function_constants=True,actual_specialized_GPU_acceptance_pending=True)
if air_manifest:report.update(native_XR_code_formats=True,XR_shader_colour_conversion=True,actual_XR_format_acceptance_pending=True)
if air_manifest:report.update(native_IOSurface_texture_import=True,actual_IOSurface_GPU_acceptance_pending=True)
if air_manifest:report.update(native_mipmaps=True,native_mip_level_CPU_regions_and_sampler_filters=True,actual_mipmap_GPU_acceptance_pending=True)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
