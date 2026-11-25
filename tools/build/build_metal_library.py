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
        'gpu/metal/MetalDepthStencilState.inc','gpu/sweetgl/MetalDepthStencilRules.h','gpu/sweetgl/MetalDepthAttachment.h','gpu/sweetgl/MetalStencilAttachment.h','gpu/metal/MetalMemoryless.inc',
        'gpu/metal/MetalMultisample.inc','gpu/sweetgl/MetalMultisampleAPI.h','gpu/sweetgl/MultisampleLayout.h',
        'gpu/metal/NativePresentation16.h','gpu/metal/MetalPresentation.inc',
        'gpu/metal/air/ESSLTarget.h','gpu/metal/air/HalfESSL.h','gpu/metal/air/Compiler.h','gpu/sweetgl/MetalShaderAPI.h','gpu/sweetgl/XRColourShader.h']
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
if air_manifest:report.update(native_reflected_render_buffer_selection=True,render_buffer_selection_revision=1,
    unused_encoder_bindings_retained_and_not_consumed=True,missing_required_render_buffers_rejected=True,
    actual_reflected_render_buffer_GPU_acceptance_pending=True)
if air_manifest:report.update(native_function_constants=True,actual_specialized_GPU_acceptance_pending=True)
if air_manifest:report.update(native_XR_code_formats=True,XR_shader_colour_conversion=True,actual_XR_format_acceptance_pending=True)
if air_manifest:report.update(native_IOSurface_texture_import=True,actual_IOSurface_GPU_acceptance_pending=True)
if air_manifest:report.update(native_mipmaps=True,native_mip_level_CPU_regions_and_sampler_filters=True,actual_mipmap_GPU_acceptance_pending=True)
if air_manifest:report.update(native_depth32_attachment=True,native_depth_stencil_state_objects=True,
    depth32_GPU_page_import_and_draw_clear=True,actual_depth32_GPU_acceptance_pending=True,depth_XR_ordered_pending=True,
    native_stencil8_attachment=True,stencil8_original_separate_plane_import=True,stencil8_independent_draw_clear=True,actual_stencil8_GPU_acceptance_pending=True)
if air_manifest:report.update(native_memoryless_attachments=True,memoryless_public_metadata_without_backing=True,
    memoryless_independent_per_encoder_private_storage=True,memoryless_Load_Store_CPU_sampling_rejected=True,actual_memoryless_GPU_acceptance_pending=True)
if air_manifest:report.update(native_multisample_attachments=True,native_samples=[2,4],multisample_original_linear_pages=True,
    multisample_colour_average_depth_sample0_min_max_GPU_resolve=True,actual_multisample_GPU_acceptance_pending=True,
    multisample_XR_and_stencil_resolve_pending=True,multisample_required_native_tests=20)
if air_manifest:report.update(native_drawable_presentation=True,native_drawable_descriptor_flags=True,original_drawable_time_and_options_preserved=True,
    actual_GPU_retirement_before_Scheduled_and_native_present=True,native_presentation_required_tests=21,actual_original_drawable_GPU_acceptance_pending=True,
    actual_system_UI_optical_display_completion_pending=True)
if air_manifest:report.update(native_MRT_and_XR_blend=True,independent_color_attachments_max=8,original_XR_codes_preserved_with_GPU_writeback=True,
    native_XR_ordered_FP32_image_blend=True,actual_MRT_GPU_acceptance_pending=True)
if air_manifest:
    bindings=manifest['frontend_build_binding']['source_files']
    assert bindings.get('RenderSamplerRules.h')==sha(root/'gpu/sweetgl/RenderSamplerRules.h')
    assert bindings.get('metal_render.c')==sha(root/'gpu/sweetgl/metal_render.c')
    assert manifest['frontend_build_binding'].get('multisample_backend_revision')==1
    for name in ('MetalMultisampleAPI.h','MultisampleLayout.h','MultisampleResolveGLES.h'):
        assert bindings.get(name)==sha(root/'gpu/sweetgl'/name),'Actual MSAA GLES transport header differs: '+name
    if manifest['frontend_build_binding'].get('XR_clear_decode_elision'):
        report.update(native_XR_clear_decode_elision=True,original_XR_raw_snapshot_and_Clear_writeback_preserved=True,
                      actual_XR_Clear_Load_GPU_acceptance_pending=True)
    if manifest['frontend_build_binding'].get('Clear_shader_context_cache'):
        assert bindings.get('MetalShaderInternals.h')==sha(root/'gpu/sweetgl/MetalShaderInternals.h')
        report.update(native_Clear_shader_cache=True,Clear_shader_cache_private_context_owned=True,
                      original_Clear_colours_viewport_scissor_and_draw_preserved=True,
                      actual_repeated_Clear_GPU_acceptance_pending=True)
    if manifest['frontend_build_binding'].get('IR3_variant_cache'):
        report.update(native_IR3_variant_cache=True,original_Freedreno_IR3_serializer=True,
                      exact_build_original_NIR_variant_key_bound=True,actual_IR3_cache_GPU_acceptance_pending=True)
    report.update(native_AIR_constant_samplers=True,actual_constant_sampler_GPU_acceptance_pending=True,
                  constant_sampler_scope='normalized 2D, actual border/anisotropy capabilities',
                  per_function_immutable_graphics_translation_cache=True)
    if 'program_cache.c' in bindings:
        assert bindings['program_cache.c']==sha(root/'gpu/sweetgl/program_cache.c')
        assert manifest['frontend_build_binding'].get('program_cache_build_identity')
        report.update(native_program_binary_cache=True,requires_source_scoped_cache=True,actual_fresh_context_program_binary_GPU_acceptance_pending=True,
                      immutable_program_cache_identity=manifest['frontend_build_binding']['program_cache_build_identity']['build_identity'])
if air_manifest and air_manifest.get('local_static_half_storage_reinterpretation'):
    report.update(native_static_half_bits=True,actual_static_half_GPU_acceptance_pending=True)
(out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))
