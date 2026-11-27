"""Compile the current frontend against audited, unchanged existing Mesa archives."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--mesa-build');a=p.parse_args()
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists()
out.mkdir(parents=True)
source=root/'gpu/sweetgl';mesa=root/'deps/mesa';build=Path(a.mesa_build).absolute() if a.mesa_build else root/'build/mesa-ios'
assert build.is_relative_to(root/'build') and (build/'build.ninja').exists()
lock=json.loads((root/'reproducibility/source-lock.json').read_bytes())
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
sources={str(f.relative_to(source)):sha(f) for f in source.rglob('*') if f.is_file() and not any(p in ('out','__pycache__') for p in f.relative_to(source).parts)}
for path,wanted in lock['sweetgl']['files'].items():
    if path in ('mesa-setup.sh','mesa.patch'):continue
    assert sources[path]==(lock['sweetgl']['build_script_updated_for_canonical_output'] if path=='build.sh' else wanted),path
archives=re.search(r'LIBS="([^"]+)"',(source/'build.sh').read_text()).group(1).split()
archives+=['src/mapi/shared-glapi/libglapi.0.dylib']
dependencies={path:sha(build/path) for path in archives}
compiler_overlay=None
overlay_manifest=build/'nir-callee-local-ssa-inputs.json'
if overlay_manifest.exists():
    compiler_overlay=json.loads(overlay_manifest.read_bytes())
    assert compiler_overlay['revision']==1 and compiler_overlay['link_input_overlay_only']
    assert compiler_overlay['Mesa_commit']==lock['mesa']['base']
    assert compiler_overlay['patch']=='gpu/nir-callee-local-ssa.patch'
    assert compiler_overlay['patch_sha256']==sha(root/compiler_overlay['patch'])
    assert compiler_overlay['builder_sha256']==sha(root/'tools/build/build_callee_ssa_overlay.py')
    assert compiler_overlay['source_binding_sha256']==sha(root/'reproducibility/nir-callee-local-ssa-source-binding-20261128.json')
    assert dependencies==compiler_overlay['overlay_archives']
    assert set(compiler_overlay['updated_archives'])=={'src/compiler/glsl/libglsl.a','src/compiler/nir/libnir.a'}
    for path,row in compiler_overlay['sources'].items():
        assert sha(mesa/path)==row['before_sha256']
        assert sha(build.parent/'sources'/path)==row['after_sha256']
    for row in compiler_overlay['compiled_objects'].values():
        assert row['architecture']=='arm64' and sha(root/row['object'])==row['sha256']
env=dict(os.environ,SWEETGL_MESA_SOURCE=str(mesa),SWEETGL_MESA_BUILD=str(build),SWEETGL_OUTPUT=str(out))
with (out/'build.log').open('wb') as log:
    result=subprocess.run(['bash',str(source/'build.sh')],env=env,stdout=log,stderr=subprocess.STDOUT)
(out/'build.rc').write_text(str(result.returncode)+'\n')
assert result.returncode==0,(out/'build.log').read_text(errors='replace')[-3000:]
for path,digest in sources.items():assert sha(source/path)==digest,path
for path,digest in dependencies.items():assert sha(build/path)==digest,path
binding={'source_files':sources,'mesa_dependencies':dependencies,'mesa_base':lock['mesa']['base'],
         'mesa_patch_sha256':lock['mesa']['patch_sha256'],'only_frontend_recompiled':True,
         'library_sha256':sha(out/'sweetgl.dylib'),'device_operations':0}
if compiler_overlay:
    binding['Mesa_shader_compiler_overlay']=compiler_overlay
    binding['Mesa_shader_compiler_overlay_manifest_sha256']=sha(overlay_manifest)
    binding['actual_compiler_local_SSA_GPU_acceptance_pending']=True
binding['XR_clear_decode_elision']=True
binding['Clear_shader_context_cache']=True
binding['render_target_observation_revision']=1
if 'OrderedImageBarrier.h' in sources:
    binding['ordered_XR_image_barrier_revision']=4
    binding['ordered_XR_barrier_exact_A5xx_sysmem_image_contract']=True
    binding['ordered_XR_barrier_waits_shader_and_CP_writes_and_invalidates_UCHE']=True
    binding['ordered_XR_per_primitive_rounding_and_final_fences_preserved']=True
    binding['ordered_XR_monotonic_timestamp_wait_and_separate_scratch_cell']=True
    binding['ordered_XR_requires_synchronous_GL_frontend']=True
    binding['ordered_XR_first_timestamp_scratch_initialized_by_GPU']=True
    binding['ordered_XR_dense_driver_images_resolved_from_linked_GL_program']=True
    binding['ordered_XR_unknown_images_SSBO_writes_or_atomics_use_original_barrier']=True
    binding['actual_ordered_XR_image_barrier_GPU_acceptance_pending']=True
if 'MetalDepthAttachment.h' in sources:
    binding['depth32_attachment_revision']=1
    binding['depth_stencil_state_mapping']=True
    binding['actual_depth32_GPU_acceptance_pending']=True
if 'MetalStencilAttachment.h' in sources:
    binding['stencil8_attachment_revision']=1
    binding['stencil8_original_separate_plane_import']=True
    binding['actual_stencil8_GPU_acceptance_pending']=True
if 'DepthTiledPlane.h' in sources:
    binding['depth_tiled_backend_revision']=10
    binding['depth_tiled_independent_FDL5_work_pages']=True
    binding['depth_tiled_GPU_copy_fences_and_retirement_required']=True
    binding['depth_tiled_designated_pipe_box_and_raw_copy_metadata_required']=True
    binding['depth_tiled_sysmem_mode_from_pinned_fd5_tile_mode']=3
    binding['depth_multisample_Load_Store_actual_sample_shader_bitcast']=True
    binding['stencil_clear_combined_depth_attachment_preserves_depth']=True
    binding['depth_tiled_internal_2D_native_3D_state_replay_required']=True
    binding['stencil_Load_Store_actual_texture_GPU_bitplanes']=True
    binding['stencil_Clear_GPU_zero_and_eight_bitplane_draws']=True
    binding['stencil_Load_GPU_zero_draw_required']=True
    binding['stencil_only_unused_depth_primary_native_TILE5_3']=True
    binding['stencil_only_unused_depth_private_FDL5_work_and_retirement']=True
    binding['actual_depth_tiled_GPU_acceptance_pending']=True
if 'MetalMultisampleAPI.h' in sources:
    binding['multisample_backend_revision']=1
    binding['multisample_real_sample_count_and_original_page_import']=True
    binding['actual_multisample_GPU_acceptance_pending']=True
if 'QuartzCoreMatrixLayout.h' in sources:
    binding['CA_matrix_revision']=1
    binding['GLES_suite_tests']=9
    binding['CA_matrix_original_empty_slots']=2
    binding['actual_CA_matrix_GPU_acceptance_pending']=True
binding['mesa_build_directory']=build.relative_to(root).as_posix()
if (build/'ir3-variant-cache-inputs.json').exists():
    inputs=json.loads((build/'ir3-variant-cache-inputs.json').read_bytes())
    assert inputs==lock['mesa']['IR3_variant_cache']
    binding['IR3_variant_cache']=inputs
    binding['actual_IR3_variant_cache_GPU_acceptance_pending']=True
if (out/'ProgramCacheBuildIdentity.json').exists():
    binding['program_cache_build_identity']=json.loads((out/'ProgramCacheBuildIdentity.json').read_bytes())
(out/'build-binding.json').write_bytes((json.dumps(binding,indent=2)+'\n').encode())
print(json.dumps({'output':str(out),'library_sha256':binding['library_sha256'],'source_files':len(sources),'existing_mesa_inputs':len(dependencies)}))
