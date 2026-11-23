"""Bind a read-only cache inventory to the exact GLES context and Metal scope."""
from program_request import specification as request_specification
from program_cache_scope import specification as scope_specification


def specification(ram, resources, prime=False):
    if not isinstance(prime, bool):
        raise ValueError('Inventory prime flag must be Boolean')
    context = request_specification(ram)
    scope = scope_specification(resources)
    if ram['library_sha256'] != resources['gles_library_sha256']:
        raise ValueError('Inventory GLES and Metal inputs differ')
    if resources.get('native_presentation_required_tests') != 21:
        raise ValueError('Current inventory requires complete native21')
    return dict(revision=1, context_identity=context['context_identity'],
                build_identity=context['build_identity'], scope_identity=scope['identity'],
                directory=scope['directory'], max_files=64, max_seen=4096,
                max_bytes=33554432, max_payload=8388608, max_line_bytes=160,
                max_prime_entries=8, read_only=True, bin_import_performed=False,
                conditional_prime=prime, native_groups_before_inventory=21,
                PNG_cases_before_inventory=6)


def header(spec):
    identity = bytes.fromhex(spec['context_identity'])
    if spec['revision'] != 1 or len(identity) != 32:
        raise ValueError('Inventory context identity differs')
    values = ','.join('0x%02x' % x for x in identity)
    return ('/* Generated immutable cache inventory context. */\n'
            'static const unsigned char sweet_inventory_identity[32]={' + values + '};\n').encode()
