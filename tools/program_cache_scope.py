"""Source-bound cache roots keep old missing requests outside a new primer."""
import hashlib
import json


def specification(resources):
    source = hashlib.sha256(json.dumps(resources['source_files'], sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    fields = dict(revision=1, GLES_sha256=resources['gles_library_sha256'], Metal_source_map_sha256=source)
    identity = hashlib.sha256(json.dumps(fields, sort_keys=True, separators=(',', ':')).encode()).hexdigest()
    return dict(**fields, identity=identity, directory='/private/var/tmp/sgm-'+identity[:32],
                mode=0o1777, existing_files_preserved=True)


def header(spec):
    assert spec['revision'] == 1 and len(spec['identity']) == 64
    assert spec['directory'] == '/private/var/tmp/sgm-'+spec['identity'][:32]
    return ('/* Generated source-bound cache root. */\n'
            'static const char sweet_cache_scope_id[]="'+spec['identity']+'";\n'
            'static const char sweet_cache_scope_directory[]="'+spec['directory']+'";\n').encode()
