"""Actual-file corruption checks for the pre-device compiler audit. TEMP only."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from mesa_compiler_overlay import verify


def digest(data):return hashlib.sha256(data).hexdigest()


class Overlay(unittest.TestCase):
    def setUp(self):
        # Retained in TEMP for native Recycle Bin handling; no recursive delete.
        self.root=Path(tempfile.mkdtemp(prefix='lilac-overlay-audit-'))
        self.build=self.root/'build/candidate/mesa-overlay'
        def write(relative,data):
            p=self.root/relative;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);return p
        self.write=write
        patch=b'author fixture patch';builder=b'author fixture builder'
        write('gpu/nir-callee-local-ssa.patch',patch);write('tools/build/build_callee_ssa_overlay.py',builder)
        paths=['src/compiler/glsl/gl_nir_linker.c','src/compiler/nir/nir_functions.c','src/compiler/nir/nir_lower_vars_to_ssa.c','src/compiler/nir/nir.h']
        sources={}
        for p in paths:
            a=('before '+p).encode();b=('after '+p).encode()
            write('deps/mesa/'+p,a);write('build/candidate/sources/'+p,b)
            sources[p]=dict(before_sha256=digest(a),after_sha256=digest(b))
        reference=json.dumps(dict(sources=sources)).encode()
        write('reproducibility/nir-callee-local-ssa-source-binding-20261128.json',reference)
        objects={}
        for i,p in enumerate(paths[:3]):
            name='build/candidate/objects/'+Path(p).name+'.o';data=b'\xcf\xfa\xed\xfe'+(0x0100000c).to_bytes(4,'little')+bytes([i])
            write(name,data);objects[p]=dict(object=name,sha256=digest(data),architecture='arm64')
        names=['src/compiler/glsl/libglsl.a','src/compiler/nir/libnir.a','src/mesa/libmesa.a'];old={};new={};updated={}
        for i,p in enumerate(names):
            a=('archive '+p).encode();b=a+b' changed' if i<2 else a
            write('build/base/'+p,a);write('build/candidate/mesa-overlay/'+p,b)
            old[p]=digest(a);new[p]=digest(b)
            if i<2:updated[p]=dict(before_sha256=old[p],after_sha256=new[p])
        self.overlay=dict(revision=1,link_input_overlay_only=True,device_operations=0,Mesa_commit='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de',patch='gpu/nir-callee-local-ssa.patch',patch_sha256=digest(patch),source_binding_sha256=digest(reference),builder_sha256=digest(builder),sources=sources,compiled_objects=objects,original_mesa_build='build/base',original_archives=old,overlay_archives=new,updated_archives=updated)
        self.frontend=dict(mesa_build_directory='build/candidate/mesa-overlay',mesa_dependencies=new)
        self.bind_manifest()

    def bind_manifest(self):
        raw=json.dumps(self.overlay).encode();self.write('build/candidate/mesa-overlay/nir-callee-local-ssa-inputs.json',raw)
        self.frontend['Mesa_shader_compiler_overlay']=copy.deepcopy(self.overlay)
        self.frontend['Mesa_shader_compiler_overlay_manifest_sha256']=digest(raw)

    def test_complete_actual_file_audit(self):
        result=verify(self.root,self.frontend)
        self.assertEqual((result['actual_sources_checked'],result['actual_ARM64_objects_checked'],result['actual_archives_checked']),(4,3,3))

    def test_actual_file_changes_refused(self):
        paths=['gpu/nir-callee-local-ssa.patch','tools/build/build_callee_ssa_overlay.py','deps/mesa/src/compiler/nir/nir.h','build/candidate/sources/src/compiler/nir/nir.h','build/candidate/objects/nir_functions.c.o','build/base/src/compiler/nir/libnir.a','build/candidate/mesa-overlay/src/compiler/nir/libnir.a','build/candidate/mesa-overlay/nir-callee-local-ssa-inputs.json']
        for name in paths:
            path=self.root/name;original=path.read_bytes();path.write_bytes(original+b' changed')
            with self.subTest(file=name),self.assertRaises((AssertionError,json.JSONDecodeError)):verify(self.root,self.frontend)
            path.write_bytes(original)

    def test_wrong_cpu_even_when_hash_claims_match(self):
        row=next(iter(self.overlay['compiled_objects'].values()));p=self.root/row['object']
        data=b'\xcf\xfa\xed\xfe'+(0x01000007).to_bytes(4,'little');p.write_bytes(data);row['sha256']=digest(data);self.bind_manifest()
        with self.assertRaises(AssertionError):verify(self.root,self.frontend)

    def test_legacy_frontend_has_no_overlay(self):
        self.assertIsNone(verify(self.root,{}))


if __name__=='__main__':unittest.main()
