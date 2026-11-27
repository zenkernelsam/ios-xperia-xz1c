"""Verify actual callee-SSA compile inputs, not just claimed manifest fields."""
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build_path(root, relative):
    value=Path(relative)
    assert not value.is_absolute() and value.parts[0]=='build' and '..' not in value.parts
    path=(root/value).resolve()
    assert path.is_relative_to(root/'build')
    return path


def verify(root, frontend):
    root=Path(root).resolve()
    if 'Mesa_shader_compiler_overlay' not in frontend:
        return None
    overlay=frontend['Mesa_shader_compiler_overlay']
    build=build_path(root,frontend['mesa_build_directory'])
    manifest=build/'nir-callee-local-ssa-inputs.json'
    assert sha(manifest)==frontend['Mesa_shader_compiler_overlay_manifest_sha256']
    assert json.loads(manifest.read_bytes())==overlay
    assert overlay['revision']==1 and overlay['link_input_overlay_only']
    assert overlay['device_operations']==0 and overlay['Mesa_commit']=='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de'
    patch=root/'gpu/nir-callee-local-ssa.patch'
    reference=root/'reproducibility/nir-callee-local-ssa-source-binding-20261128.json'
    assert overlay['patch']=='gpu/nir-callee-local-ssa.patch' and sha(patch)==overlay['patch_sha256']
    assert sha(reference)==overlay['source_binding_sha256']
    assert sha(root/'tools/build/build_callee_ssa_overlay.py')==overlay['builder_sha256']
    assert overlay['sources']==json.loads(reference.read_bytes())['sources']
    for relative,binding in overlay['sources'].items():
        assert sha(root/'deps/mesa'/relative)==binding['before_sha256']
        assert sha(build.parent/'sources'/relative)==binding['after_sha256']
    assert len(overlay['compiled_objects'])==3
    for binding in overlay['compiled_objects'].values():
        obj=build_path(root,binding['object'])
        assert binding['architecture']=='arm64' and sha(obj)==binding['sha256']
        head=obj.read_bytes()[:8]
        assert head[:4]==b'\xcf\xfa\xed\xfe' and int.from_bytes(head[4:8],'little')==0x0100000c
    original=build_path(root,overlay['original_mesa_build'])
    expected=frontend['mesa_dependencies']
    assert overlay['overlay_archives']==expected and set(overlay['original_archives'])==set(expected)
    for relative,digest in expected.items():
        assert sha(build/relative)==digest
        assert sha(original/relative)==overlay['original_archives'][relative]
    updated=overlay['updated_archives']
    assert set(updated)=={'src/compiler/glsl/libglsl.a','src/compiler/nir/libnir.a'}
    assert {p for p in expected if expected[p]!=overlay['original_archives'][p]}==set(updated)
    for relative,binding in updated.items():
        assert binding['before_sha256']==overlay['original_archives'][relative]
        assert binding['after_sha256']==expected[relative]
    return dict(actual_sources_checked=4,actual_ARM64_objects_checked=3,
                actual_archives_checked=len(expected),original_archives_unchanged=True,
                overlay_manifest_sha256=sha(manifest),device_operations=0)
