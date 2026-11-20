"""Strict metadata for the independently reverified full native hardware stage."""
import hashlib,json,re
NATIVE_TESTS=('Metal_resources','Metal_queue','AIR_compute','AIR_render','AIR_constants','AIR_XR','Metal_IOSurface','Metal_mipmaps')
HEX_FIELDS=('kernel_sha256','gles_library_sha256','metal_library_sha256','metal_source_files_sha256','native_run_image_sha256','native_run_log_sha256')
def digest(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()
def validate(proof,resources=None):
    keys={'revision','version','native_run_token','native_run_process','native_tests',*HEX_FIELDS}
    assert set(proof)==keys,'Native proof fields differ from revision1 schema'
    assert proof['revision']==1 and type(proof['version']) is int and proof['version']>=729
    extensions=('AIR_narrow','AIR_half','Metal_MRT','AIR_sampler','Program_cache','AIR_static_half','Clear_cache','IR3_cache','Metal_depth32','Metal_stencil8','Metal_memoryless')
    assert proof['native_run_process']=='backboardd' and proof['native_tests'] in [list(NATIVE_TESTS)+list(extensions[:n]) for n in range(len(extensions)+1)]
    assert isinstance(proof['native_run_token'],str) and re.fullmatch('[0-9a-f]{16}',proof['native_run_token'])
    for name in HEX_FIELDS:assert isinstance(proof[name],str) and re.fullmatch('[0-9a-f]{64}',proof[name]),name
    if resources is not None:
        if resources.get('AIR_frontend',{}).get('local_narrow_conversion_adaptation'):assert 'AIR_narrow' in proof['native_tests'],'New AIR conversion needs its own complete hardware proof'
        if resources.get('AIR_frontend',{}).get('local_half_storage_and_rounding_adaptation'):assert 'AIR_half' in proof['native_tests'],'New half adaptation needs its own complete hardware proof'
        if resources.get('native_MRT_and_XR_blend'):assert 'Metal_MRT' in proof['native_tests'],'New MRT/XR blend needs complete original-page hardware proof'
        if resources.get('native_XR_clear_decode_elision'):assert 'Metal_MRT' in proof['native_tests'],'XR Clear optimization needs original-page/mask/guard hardware proof'
        if resources.get('native_AIR_constant_samplers'):assert 'AIR_sampler' in proof['native_tests'],'New literal sampler needs its own original-page GPU proof'
        if resources.get('native_program_binary_cache'):assert 'Program_cache' in proof['native_tests'],'Program binary restore needs its own fresh-context GPU proof'
        if resources.get('native_static_half_bits'):assert 'AIR_static_half' in proof['native_tests'],'Static half memory aliases need their own complete hardware proof'
        if resources.get('native_Clear_shader_cache'):assert 'Clear_cache' in proof['native_tests'],'Clear shader reuse needs its own complete original-page hardware proof'
        if resources.get('native_IR3_variant_cache'):assert 'IR3_cache' in proof['native_tests'],'IR3 variant restore needs its own complete original-page hardware proof'
        if resources.get('native_depth32_attachment'):assert 'Metal_depth32' in proof['native_tests'],'Depth32 attachments need their own original-page GPU/guard/compare proof'
        if resources.get('native_stencil8_attachment'):assert 'Metal_stencil8' in proof['native_tests'],'Stencil8 attachments need their own original-page front/back/guard/compare proof'
        if resources.get('native_memoryless_attachments'):assert 'Metal_memoryless' in proof['native_tests'],'Memoryless attachments need their own original pass-page/guard/queue-isolation proof'
        assert proof['gles_library_sha256']==resources['gles_library_sha256'],'Native prior GLES differs'
        assert proof['metal_library_sha256']==resources['library_sha256'],'Native prior Metal differs'
        assert proof['metal_source_files_sha256']==digest(resources['source_files']),'Native prior Metal sources differ'
    return proof
def header(proof):
    validate(proof)
    names={'native_run_token':'SWEET_NATIVE_RUN_TOKEN','native_run_log_sha256':'SWEET_NATIVE_LOG_SHA',
        'kernel_sha256':'SWEET_NATIVE_KERNEL_SHA','gles_library_sha256':'SWEET_NATIVE_GLES_SHA','metal_library_sha256':'SWEET_NATIVE_METAL_SHA'}
    return ('/* Exact prior full-hardware stage; reverified before dispatch. */\n'+
        ''.join('#define '+macro+' '+json.dumps(proof[key])+'\n' for key,macro in names.items())+
        '#define SWEET_NATIVE_PROOF_SHA '+json.dumps(digest(proof))+'\n').encode()
