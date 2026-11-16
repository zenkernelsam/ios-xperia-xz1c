"""Immutable FD540 request-export identity shared by build and RAM gates."""
import hashlib
import struct

FRAGMENT_BYTES = (86136, 45773)
VERTEX_BYTES = 4840
CONFIG_BYTES = 316
EXPORT_LIMIT = 262144


def specification(ram, framing_revision=1):
    build = ram['frontend_build_binding']['program_cache_build_identity']['build_identity']
    raw = bytes.fromhex(build)
    assert len(raw) == 32
    # Pinned Mesa menums.h: API_OPENGLES2=2; sweetgl_create_metal requests ES3.1.
    identity = hashlib.sha256(raw + struct.pack('<II', 2, 31) + b'FD540').hexdigest()
    spec = dict(revision=1, build_identity=build, context_identity=identity,
                api=2, version=31, renderer='FD540', vertex_bytes=VERTEX_BYTES,
                fragment_bytes=list(FRAGMENT_BYTES), max_records=2,
                max_bytes=EXPORT_LIMIT, read_only=True, no_compile=True)
    assert framing_revision in (1, 2)
    if framing_revision == 2:
        spec.update(framing_revision=2, chunk_bytes=32, max_line_bytes=180)
    return spec


def header(spec):
    assert spec['revision'] == 1 and spec['vertex_bytes'] == VERTEX_BYTES
    assert spec['fragment_bytes'] == list(FRAGMENT_BYTES)
    identity = bytes.fromhex(spec['context_identity'])
    assert len(identity) == 32
    values = ','.join('0x%02x' % x for x in identity)
    return ('/* Generated immutable request identity. */\n'
            'static const unsigned char sweet_request_identity[32]={' + values + '};\n').encode()
