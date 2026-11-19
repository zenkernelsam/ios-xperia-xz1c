"""Byte-bound read-only RAM frontend packages, independent of the boot transport."""
import hashlib,json,struct,zlib,re
from pathlib import Path
from native_proof import validate as validate_native,header as native_header
from program_request import specification as request_specification,header as request_header
from program_cache_scope import specification as cache_scope_specification,header as cache_scope_header
from png_encoder_proof import validate as validate_png,header as png_header
ROOT=Path(__file__).resolve().parents[1]
API='a8f1e5a6693c82fbf34c438868723bcd61cc30f1829e57f17519277dba252bb3'
HELPER='e65bdc7b70d098725918c637cc8cb1c25a8a544186d0f4468b399fda52024898'
BASELINE={'library_sha256':'9a59820c7e54b1626cacd49acb70a7d460be3c01c415a17429a2464bb5b11571',
          'library_bytes':14675424,'library_fnv64':'bb763317000ee0d7'}
def sha(data):return hashlib.sha256(data).hexdigest()
def fnv(data):
    value=14695981039346656037
    for byte in data:value=((value^byte)*1099511628211)&0xffffffffffffffff
    return f'{value:016x}'
def load(folder,expected=None):
    folder=Path(folder).resolve();assert folder.is_relative_to(ROOT/'build')
    manifest_bytes=(folder/'manifest.json').read_bytes();m=json.loads(manifest_bytes)
    assert m['read_only_verified'] and m['aliases_m_g_verified']
    assert m['api_sha256']==API and m['helper_sha256']==HELPER
    blob=(folder/'glram.zblob').read_bytes()
    assert len(blob)==m['blob_bytes'] and sha(blob)==m['blob_sha256'] and len(blob)<=8<<20
    magic,version,header,raw_bytes,packed_bytes,raw_fnv=struct.unpack_from('<8sIIQQQ',blob)
    assert (magic,version,header)==(b'SGLZ0001',1,40)
    assert raw_bytes==m['raw_bytes']==32<<20 and packed_bytes==len(blob)-40
    decoder=zlib.decompressobj();raw=decoder.decompress(blob[40:],raw_bytes+1)
    assert len(raw)==raw_bytes and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert raw[1024:1026]==b'H+' and sha(raw)==m['raw_sha256']
    assert f'{raw_fnv:016x}'==m['raw_fnv64'] and fnv(raw)==m['raw_fnv64']
    names={'sweetgl.dylib','libglapi.0.dylib','georead.dylib'}
    if 'exception_observer' in m:
        observer=m['exception_observer'];names.add('exception_observe.dylib')
        assert observer['target_process'] in ('AegirPoster','backboardd') and observer['observations_max']==16
        assert observer['native_original_chain_and_bound_test_passed'] and observer['host_asan_ubsan_passed']
        assert m['observer_alias_e_verified']
        assert observer['source']=='gpu/diagnostics/exception_observe.m'
        if 'renderer_observations_max' in observer:
            assert observer['renderer_observations_max']==32 and observer['renderer_abi_and_device_observer_test_passed']
            assert observer['metadata_only_no_buffer_allocation'] and observer['original_renderer_return_and_exception_preserved']
        if 'runtime_Metal_ABI_methods' in observer:
            assert observer['runtime_Metal_ABI_methods']==22 and observer['metadata_observer_no_device_calls_test_passed']
            sources={'gpu/diagnostics/exception_observe.m','gpu/diagnostics/MetalRuntimeObserve.h'}
            if observer.get('probe_startup_revision'):
                assert observer['probe_startup_revision']==1 and observer['actual_startup_readiness_host_asan_ubsan_passed']
                sources.add('gpu/diagnostics/MetalProbeStartup.h')
            if observer.get('original_pipeline_observer'):
                sources.add('gpu/diagnostics/MetalPipelineObserve.h')
                assert observer.get('registration_only') and observer['actual_factory_return_error_pointer_exception_forwarding_host_passed']
            if observer.get('original_postdraw_observer'):
                sources.add('gpu/diagnostics/PostDrawObserve.h')
                assert observer.get('registration_only') and observer.get('original_pipeline_observer') and observer['target_process']=='AegirPoster'
                assert observer['postdraw_observer_revision'] in (1,2,3) and observer['postdraw_observations_max']==64
                assert observer['postdraw_apis']==['png','add_image','finalize','colorsync']
                assert observer['postdraw_return_nil_throw_errno_nesting_host_passed'] and observer['postdraw_real_PNG_bytes_pixels_ICC_input_host_passed']
                assert observer['postdraw_original_pixel_and_profile_options_preserved'] and observer['postdraw_durations_inclusive_not_additive']
                assert re.fullmatch('[0-9a-f]{64}',observer['postdraw_host_test_source_sha256'])
                if observer['postdraw_observer_revision']>=2:
                    assert observer['postdraw_png_input_metadata'] and observer['png_input_getters_exceptions_errno_host_passed']
                    assert observer['png_input_no_pixel_or_profile_reads_host_passed']
                    assert re.fullmatch('[0-9a-f]{64}',observer['png_input_host_test_source_sha256'])
                else:assert not observer.get('postdraw_png_input_metadata')
                if observer['postdraw_observer_revision']==3:
                    sources|={'gpu/compat/PNGImageIOEncoder.h','gpu/compat/PNGImageDispatch.h','tools/png_encoder_proof.py'}
                    assert observer['png_filter_encoder'] and observer['png_encoder_fallback_nil_throw_errno_input_metadata_host_passed']
                    assert observer['png_encoder_preserves_pixels_profile_metadata'] and observer['png_encoder_changes_only_encoding_path_and_compression']
                    assert observer['png_encoder_postdraw_composition_host_passed']
                    assert observer['png_imageio_encoder_source_sha256']==observer['source_files']['gpu/compat/PNGImageIOEncoder.h']
                    p=validate_png(observer['png_encoder_prior_proof'],observer['png_imageio_encoder_source_sha256'],observer['metal_resources']['gles_library_sha256'],observer['metal_resources']['library_sha256'])
                    assert observer['png_encoder_proof_header_sha256']==sha(png_header(p)) and re.fullmatch('[0-9a-f]{64}',observer['png_encoder_host_test_source_sha256'])
            if observer.get('snapshot_method_observer'):
                sources.add('gpu/diagnostics/SnapshotMethodObserve.h')
                assert observer.get('original_postdraw_observer') and observer['snapshot_method_observer_revision']==1
                assert observer['snapshot_method_count_max']==8 and observer['snapshot_method_apis']==['pipeline','library_url','library_bundle','snapshot','wait','execute','read','read_full']
                assert observer['snapshot_method_exact_ABI_arguments_returns_errors_errno_throws_host_passed']
                assert observer['snapshot_method_read_bytes_guards_and_ABI_owner_repeat_rejection_host_passed']
                assert observer['snapshot_method_target_coverage_requires_actual_metadata']
                assert re.fullmatch('[0-9a-f]{64}',observer['snapshot_method_host_test_source_sha256'])
            if observer.get('cif10_PNG_reference'):
                sources.add('gpu/diagnostics/CIF10PNGReferenceProbe.h')
                assert observer['target_process']=='backboardd' and observer['cif10_reference_revision'] in (1,2,3)
                assert observer['cif10_reference_cases']==(6 if observer['cif10_reference_revision']==3 else 5)
                assert observer['cif10_reference_author_pixels_only'] and observer['cif10_reference_original_UIImage_API']
                assert observer['cif10_reference_converter_not_installed'] and observer['cif10_reference_host_fixture_bounds_nil_throw_input_passed']
                assert re.fullmatch('[0-9a-f]{64}',observer['cif10_reference_host_test_source_sha256'])
                if observer['cif10_reference_revision']>=2:
                    sources.add('gpu/diagnostics/CIF10VImageCompare.h')
                    assert observer['cif10_reference_PNG_exports'] and observer['cif10_reference_vImage_modes']==2 and observer['cif10_reference_vImage_guard_buffer_host_passed']
                if observer['cif10_reference_revision']==3:
                    sources.add('gpu/diagnostics/CIF10PNGFilterCompare.h')
                    assert observer['cif10_reference_filter_policies']==3 and observer['cif10_reference_lossless_filter_compare'] and observer['cif10_reference_candidate_differences_telemetry']
                    if observer.get('cif10_reference_shared_encoder'):
                        sources.add('gpu/compat/PNGImageIOEncoder.h')
                        assert observer['png_imageio_encoder_source_sha256']==observer['source_files']['gpu/compat/PNGImageIOEncoder.h']
            if observer['target_process']=='backboardd':
                sources.add('gpu/sweetgl/sweetgl.h')
                assert observer.get('background_full_suite') and observer['waits_for_same_process_GLES_suite'] and observer['original_Aegir_not_modified_or_registered']
                assert observer.get('probe_startup_revision')==1 and not observer.get('original_Metal_registry_probe') and 'renderer_observations_max' not in observer
                assert all(observer.get(key) for key in ('metal_resource_probe','metal_queue_probe','AIR_compute_probe','AIR_render_probe','AIR_constants_probe','AIR_XR_probe','Metal_IOSurface_probe','Metal_mipmap_probe'))
            if observer.get('metal_resource_probe'):sources|={'gpu/diagnostics/MetalResourceProbe.h','gpu/metal/MetalResources.h'}
            if observer.get('metal_queue_probe'):
                sources.add('gpu/diagnostics/MetalQueueProbe.h')
                assert observer['queue_probe_submits_actual_GPU_commands'] and observer['metal_resources']['native_blit_command_queue']
            if observer.get('AIR_compute_probe'):
                sources.add('gpu/diagnostics/MetalComputeProbe.h')
                assert observer['metal_resources']['AIR_compute_pipeline'] and observer['AIR_fixture']['authored_semantic_fixture']
                assert re.fullmatch('[0-9a-f]{64}',observer['AIR_fixture']['library_sha256'])
            if observer.get('AIR_render_probe'):
                sources.add('gpu/diagnostics/MetalRenderProbe.h')
                assert observer['metal_resources']['native_texture_render_pipeline'] and observer['AIR_render_fixture']['authored_semantic_fixture']
                assert observer['AIR_render_fixture']['fixture']=='render' and re.fullmatch('[0-9a-f]{64}',observer['AIR_render_fixture']['library_sha256'])
            if observer.get('AIR_constants_probe'):
                sources.add('gpu/diagnostics/MetalConstantsProbe.h')
                assert observer['metal_resources']['native_function_constants'] and observer['AIR_constants_fixture']['authored_semantic_fixture']
                assert observer['AIR_constants_fixture']['fixture']=='constants' and re.fullmatch('[0-9a-f]{64}',observer['AIR_constants_fixture']['library_sha256'])
            if observer.get('original_Metal_registry_probe'):
                sources.add('gpu/diagnostics/MetalDiscoveryProbe.h')
                assert observer['registry_probe_process']=='AegirPoster' and observer['registry_functions_not_replaced']
                if observer.get('registration_only'):
                    validate_native(observer['prior_native_hardware_proof'],observer['metal_resources'])
                    assert observer['target_process']=='AegirPoster' and observer['long_native_probes_not_in_constructor']
                    assert not any(observer.get(k) for k in ('background_full_suite','metal_resource_probe','metal_queue_probe','AIR_compute_probe','AIR_render_probe','AIR_constants_probe','AIR_XR_probe','Metal_IOSurface_probe','Metal_mipmap_probe'))
                    assert observer['native_proof_header_sha256']==sha(native_header(observer['prior_native_hardware_proof']))
                    sources.add('gpu/metal/MetalResources.h')
                else:assert observer.get('AIR_constants_probe') and observer.get('AIR_render_probe')
            if observer.get('AIR_XR_probe'):
                sources|={'gpu/diagnostics/MetalXRProbe.h','gpu/sweetgl/XRColourTables.h'}
                assert observer.get('AIR_render_probe')
            if observer.get('Metal_IOSurface_probe'):
                sources|={'gpu/diagnostics/MetalSurfaceProbe.h','gpu/sweetgl/MetalBackingAPI.h'}
                assert observer.get('AIR_render_probe') and observer['metal_resources']['native_IOSurface_texture_import']
            if observer.get('Metal_mipmap_probe'):
                sources|={'gpu/diagnostics/MetalMipProbe.h','gpu/sweetgl/MetalShaderAPI.h','gpu/sweetgl/XRColourTables.h'}
                assert observer.get('AIR_render_probe') and observer['metal_resources']['native_mipmaps']
            if observer.get('AIR_narrow_probe'):
                sources|={'gpu/diagnostics/MetalNarrowProbe.h','gpu/diagnostics/NarrowProbeValues.h'}
                assert observer['AIR_narrow_fixture']['fixture']=='narrow' and observer['AIR_narrow_fixture']['authored_semantic_fixture']
                assert observer['metal_resources']['AIR_frontend'].get('local_narrow_conversion_adaptation')
            if observer.get('AIR_half_probe'):
                sources|={'gpu/diagnostics/MetalHalfProbe.h','gpu/diagnostics/HalfProbeValues.h'}
                assert observer.get('AIR_narrow_probe') and observer['AIR_half_fixture']['fixture']=='half' and observer['AIR_half_fixture']['authored_semantic_fixture']
                assert observer['metal_resources']['AIR_frontend'].get('local_half_storage_and_rounding_adaptation')
            if observer.get('AIR_MRT_probe'):
                sources|={'gpu/diagnostics/MetalMRTProbe.h','gpu/diagnostics/MRTProbeValues.h','gpu/sweetgl/XRColourTables.h'}
                assert observer.get('AIR_half_probe') and observer['AIR_MRT_fixture']['fixture']=='mrt' and observer['AIR_MRT_fixture']['authored_semantic_fixture']
                assert observer['metal_resources'].get('native_MRT_and_XR_blend')
            if observer.get('AIR_sampler_probe'):
                sources.add('gpu/diagnostics/MetalSamplerProbe.h')
                assert observer.get('AIR_MRT_probe') and observer['AIR_sampler_fixture']['fixture']=='sampler'
                assert observer['AIR_sampler_fixture']['authored_semantic_fixture'] and observer['metal_resources'].get('native_AIR_constant_samplers')
            if observer.get('program_cache_probe'):
                assert observer.get('AIR_sampler_probe') and observer['metal_resources'].get('native_program_binary_cache')
            if observer.get('program_cache_prime'):
                assert observer.get('program_cache_probe') and observer.get('target_process')=='backboardd'
            if observer.get('program_cache_scope'):
                sources|={'gpu/diagnostics/ProgramCacheScope.h','tools/program_cache_scope.py'}
                assert observer['metal_resources'].get('requires_source_scoped_cache') and observer['metal_resources'].get('native_program_binary_cache')
                spec=cache_scope_specification(observer['metal_resources'])
                spec['identity_header_sha256']=sha(cache_scope_header(spec))
                assert observer['program_cache_scope']==spec
            if observer.get('program_request_export'):
                sources|={'gpu/diagnostics/ProgramRequestExport.h','gpu/sweetgl/ProgramCacheCodec.h','gpu/sweetgl/MetalShaderAPI.h','tools/program_request.py'}
                assert observer.get('program_cache_probe') and observer.get('AIR_static_half_probe') and not observer.get('program_cache_prime')
                assert observer['target_process']=='backboardd'
                spec=request_specification(m,observer['program_request_export'].get('framing_revision',1))
                spec['identity_header_sha256']=sha(request_header(spec))
                assert observer['program_request_export']==spec
            if observer.get('AIR_static_half_probe'):
                sources.add('gpu/diagnostics/MetalStaticHalfProbe.h')
                assert observer.get('AIR_sampler_probe') and observer['AIR_static_half_fixture']['fixture']=='static-half'
                assert observer['AIR_static_half_fixture']['authored_semantic_fixture'] and observer['metal_resources'].get('native_static_half_bits')
            if observer.get('Clear_cache_probe'):
                sources|={'gpu/diagnostics/MetalClearCacheProbe.h','gpu/diagnostics/ClearCacheProbeValues.h'}
                assert observer.get('AIR_static_half_probe') and observer['target_process']=='backboardd'
                assert observer['Clear_cache_phases']==2 and observer['Clear_cache_targets']==8
                assert observer['metal_resources'].get('native_Clear_shader_cache')
            if observer.get('IR3_cache_probe'):
                sources.add('gpu/diagnostics/MetalIR3CacheProbe.h')
                assert observer.get('Clear_cache_probe') and observer.get('AIR_constants_probe') and observer['IR3_cache_phases']==3
                assert observer['metal_resources'].get('native_IR3_variant_cache')
            assert set(observer['source_files'])==sources
            assert observer['source_files'][observer['source']]==observer['source_sha256']
    if 'metal_resources' in m:
        names.add('MetalResources.dylib');resources=m['metal_resources']
        assert (m['exception_observer'].get('metal_resource_probe') or m['exception_observer'].get('registration_only')) and m['exception_observer']['metal_resources']==resources
        if resources.get('native_static_half_bits'):assert observer.get('registration_only') or observer.get('AIR_static_half_probe'),'New static-half RAM package lacks its own probe'
        if resources.get('native_Clear_shader_cache'):assert observer.get('registration_only') or observer.get('Clear_cache_probe'),'New Clear cache RAM package lacks its own repeated-clear GPU probe'
        if resources.get('native_IR3_variant_cache'):assert observer.get('registration_only') or observer.get('IR3_cache_probe'),'New IR3 cache RAM package lacks its own hardware-restore GPU probe'
        assert resources['only_explicit_resource_factory'] and resources['system_Metal_plugin_not_advertised']
        if m['frontend_build_binding'].get('XR_clear_decode_elision'):
            assert resources.get('native_XR_clear_decode_elision') and resources['original_XR_raw_snapshot_and_Clear_writeback_preserved']
        assert m['files']['MetalResources.dylib']['sha256']==resources['library_sha256']
        assert m['library_sha256']==resources['gles_library_sha256']
        if resources.get('AIR_compute_pipeline'):
            air=resources['AIR_frontend'];assert air['actual_AIR_stage_translation_verified'] and air['forced_legacy_interface_host_test_passed']
            assert not air['llvm_import_audit']['missing_target_imports']
        assert m['files']['exception_observe.dylib']['sha256']==observer['library_sha256']
        if resources.get('requires_source_scoped_cache'):assert observer.get('program_cache_scope'),'Source-scoped resource package lacks its matching root'
    assert set(m['files'])==names
    for name,info in m['files'].items():
        offset=info['raw_offset'];size=info['bytes']
        assert type(offset) is int and type(size) is int and offset>=0 and size>0 and offset+size<=len(raw)
        data=(folder/name).read_bytes()
        assert len(data)==size and sha(data)==info['sha256'] and raw[offset:offset+size]==data
    library=(folder/'sweetgl.dylib').read_bytes()
    assert sha(library)==m['library_sha256'] and len(library)==m['library_bytes'] and fnv(library)==m['library_fnv64']
    assert m['files']['libglapi.0.dylib']['sha256']==API and m['files']['georead.dylib']['sha256']==HELPER
    source=m['frontend_build_binding']['source_files']
    for path,digest in source.items():
        path=Path(path);assert not path.is_absolute() and '..' not in path.parts
        assert re.fullmatch('[0-9a-f]{64}',digest)
    binding={key:m[key] for key in ('library_sha256','library_bytes','library_fnv64','blob_sha256','raw_sha256')}
    binding['manifest_sha256']=sha(manifest_bytes)
    if expected is not None:assert binding==expected,'RAM library binding differs'
    return blob,binding,m
def record_library(record):return record.get('ram_library_binding',BASELINE)
def require_same_library(record,proof):
    assert record_library(record)==proof.get('ram_library_binding',BASELINE),'RAM CPU prerequisite library differs'
