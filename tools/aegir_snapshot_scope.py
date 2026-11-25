"""Stable original snapshot namespaces bound to the actual GPU and PNG sources."""

import hashlib
import json

REVISION = 1
AEGIR_SHA = "a4abba1e22350a5c577dc3ce551da90ab42606d872509cd50859ef63751454a7"
AEGIR_UUID = "fc5540ca94403f16b365dc94e0701fda"
CALLER_OFFSET = 0x63b0


def specification(resources, encoder_sha256, scope_source_sha256, observation_source_sha256=None):
    fields = dict(
        revision=REVISION, Aegir_sha256=AEGIR_SHA,
        GLES_sha256=resources["gles_library_sha256"],
        Metal_sha256=resources["library_sha256"],
        Metal_source_map_sha256=hashlib.sha256(json.dumps(
            resources["source_files"], sort_keys=True, separators=(",", ":")).encode()).hexdigest(),
        PNG_encoder_sha256=encoder_sha256, scope_source_sha256=scope_source_sha256,
    )
    if observation_source_sha256 is not None:
        if len(observation_source_sha256) != 64 or any(c not in "0123456789abcdef" for c in observation_source_sha256):
            raise ValueError("Snapshot observation source identity differs")
        fields["observation_source_sha256"] = observation_source_sha256
    identity = hashlib.sha256(json.dumps(fields, sort_keys=True, separators=(",", ":")).encode()).hexdigest()
    return dict(**fields, identity=identity, component="sweet-aegir-" + identity[:32],
                exact_Aegir_UUID=AEGIR_UUID, original_caller_offset=CALLER_OFFSET,
                directory_mode=0o700, original_caches_root_preserved=True)


def header(spec):
    if (spec["revision"] != REVISION or spec["component"] != "sweet-aegir-" + spec["identity"][:32]
            or spec["exact_Aegir_UUID"] != AEGIR_UUID or spec["original_caller_offset"] != CALLER_OFFSET):
        raise ValueError("Original Aegir namespace identity differs")
    return ('/* Exact GPU/PNG source namespace; original cache files are preserved. */\n'
            'static const char sweet_aegir_scope_id[]="' + spec["identity"] + '";\n'
            'static const char sweet_aegir_scope_component[]="' + spec["component"] + '";\n').encode()
