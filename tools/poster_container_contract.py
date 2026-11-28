"""Bound the optional readonly collector; this does not admit a signed RAM."""
SOURCES=frozenset({
    'gpu/diagnostics/PosterContainerSnapshot.h',
    'gpu/diagnostics/PosterConfigSnapshot.h',
    'gpu/sweetgl/ProgramCacheCodec.h','gpu/sweetgl/MetalShaderAPI.h',
    'tools/build/test_poster_container_snapshot.c',
    'tools/build/check_poster_container_snapshot.py',
    'tools/device/poster_container_evidence.py',
    'tools/poster_container_contract.py',
})
FIELDS={
    'poster_container_snapshot_revision':2,
    'poster_container_snapshot_roots':4,
    'poster_container_snapshot_max_entries':256,
    'poster_container_snapshot_max_files':128,
    'poster_container_snapshot_max_bytes':262144,
    'poster_container_snapshot_readonly':True,
    'poster_container_snapshot_metadata_only':True,
    'poster_container_snapshot_after_native21_PNG6':True,
    'poster_container_snapshot_changes_UI_policy':False,
    'poster_container_snapshot_host_FS_and_decoder_checked':True,
    'poster_container_snapshot_failure_changes_verdict':True,
}


def validate(observer):
    for key,value in FIELDS.items():
        assert type(observer.get(key)) is type(value) and observer[key]==value, 'Container contract differs: '+key
    assert observer.get('target_process')=='backboardd'
    assert observer.get('background_full_suite') and not observer.get('registration_only')
    assert observer.get('presentation_probe') and observer.get('cif10_PNG_reference')
    assert observer.get('cif10_reference_cases')==6
    assert not observer.get('poster_config_snapshot_revision'), 'One metadata collector per boot'
    assert not observer.get('program_cache_prime'), 'Container observation must not prime programs'
    assert SOURCES.issubset(observer['source_files']), 'Container source bindings incomplete'
    return SOURCES
