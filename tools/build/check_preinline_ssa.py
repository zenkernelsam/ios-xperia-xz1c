"""Profile pinned GLSL local-SSA lowering before inlining, in isolated host objects.

Reuse immutable native Mesa objects, including relocated native Trash inputs.
Only output objects/libraries in a new formal build directory. Never modifies
the source/build inputs or the production Mac Mesa/GLES/kernel.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

ROOT=Path(__file__).resolve().parents[2]
BASE_SHA='c94f11dbbb5284c82caab9fc86e1b20c34b3c8b84aa9ecc914dac6a517e18dcc'


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--mesa-source',required=True);parser.add_argument('--mesa-build',required=True);parser.add_argument('--shaders',required=True);parser.add_argument('--output',required=True);parser.add_argument('--callee',action='store_true');parser.add_argument('--straightline',action='store_true');parser.add_argument('--validate',action='store_true');args=parser.parse_args()
    assert not args.straightline or args.callee
    source,build,shaders,out=(Path(p).resolve() for p in (args.mesa_source,args.mesa_build,args.shaders,args.output))
    assert out.is_relative_to(ROOT/'build') and not out.exists()
    original=source/'src/compiler/glsl/gl_nir_linker.c';assert sha(original)==BASE_SHA
    assert sha(source/'src/compiler/nir/nir_lower_vars_to_ssa.c')=='f49286fb4332b62a32946587dad3e6fb0a74e1fb4e8ab72981a6a505d047dcd3'
    rows=json.loads((build/'compile_commands.json').read_bytes())
    row=next(r for r in rows if r['file'].endswith('/gl_nir_linker.c'))
    old_build=Path(row['directory']);old_source=old_build.parent/'mesa-src'
    machines=json.loads((build/'meson-info/intro-machines.json').read_bytes())
    assert machines['host']['cpu_family']==machines['build']['cpu_family']
    def relocate(value):
        return value.replace(str(old_build),str(build)).replace(str(old_source),str(source))
    def compile_flags(filename):
        item=next(r for r in rows if r['file'].endswith('/'+filename))
        flags=shlex.split(item['command']);flags=flags[:flags.index('-MD')]
        for i,v in enumerate(flags):
            if v.startswith('-I'):
                p=Path(v[2:]);p=p if p.is_absolute() else (old_build/p).resolve()
                flags[i]='-I'+relocate(str(p))
            else:flags[i]=relocate(v)
        assert '-O3' in flags and '-DNDEBUG' in flags
        return flags
    vertex=shaders/'replayed.vert';fragment=shaders/'replayed.frag'
    assert sha(vertex)=='48d760e8e35c40fe4ed5ec5908c61e4d2496280488bc7ef1061281cf27e91e89'
    assert sha(fragment)=='2b22815b2606a521eb6d6fc1a24b2cdc2b1a9cdfdb406d62322b8912812a2bf7'
    protected=[original,source/'src/compiler/nir/nir_lower_vars_to_ssa.c',build/'src/compiler/glsl/libglsl.a',build/'src/compiler/nir/libnir.a',build/'compile_commands.json',vertex,fragment]
    before={str(p):sha(p) for p in protected};out.mkdir(parents=True)
    (out/'tool-source.py').write_bytes(Path(__file__).read_bytes())
    commands=[]
    def run(command,name,cwd=build,environment=None,require_success=True):
        result=subprocess.run(list(map(str,command)),cwd=cwd,capture_output=True,env=environment,timeout=90)
        (out/(name+'.stdout')).write_bytes(result.stdout);(out/(name+'.stderr')).write_bytes(result.stderr)
        commands.append(dict(name=name,command=list(map(str,command)),returncode=result.returncode,cwd=str(cwd)))
        if require_success:assert result.returncode==0,result.stderr.decode(errors='replace')
        return result
    link_text=run(['ninja','-C',build,'-t','commands','src/gallium/targets/osmesa/libOSMesa.so.8.0.0'],'original-link-command').stdout.decode().splitlines()[-1]
    base_link=[relocate(v) for v in shlex.split(link_text)]
    needle=b'   NIR_PASS(_, shader, nir_lower_returns);\n   NIR_PASS(_, shader, nir_inline_functions);'
    actual=original.read_bytes();assert actual.count(needle)==1
    new=actual.replace(needle,b'   NIR_PASS(_, shader, nir_lower_returns);\n   NIR_PASS(_, shader, nir_opt_deref);\n   NIR_PASS(_, shader, nir_lower_vars_to_ssa);\n   NIR_PASS(_, shader, nir_inline_functions);')
    validation_objects=[];validation_api=None
    if args.validate:
        validation_api=out/'HostValidationAPI.h';validation_api.write_bytes(b'#include "nir.h"\nvoid lilac_host_validate_stage(nir_shader *shader, const char *where);\n')
        helper=out/'HostValidation.c';helper.write_bytes(b'#include "HostValidationAPI.h"\n#include <stdio.h>\nuint32_t nir_debug=0;\nvoid lilac_host_validate_stage(nir_shader *shader,const char *where){nir_validate_shader(shader,where);nir_validate_ssa_dominance(shader,where);fprintf(stderr,"HOST_NIR_VALIDATED stage=%u where=%s structure=1 dominance=1\\n",shader->info.stage,where);}\n')
        flags=[v for v in compile_flags('nir_validate.c') if v not in ('-DNDEBUG','-O3') and not v.startswith('-DMESA_DEBUG=')]
        flags+=['-DDEBUG','-DMESA_DEBUG=1','-O1','-g','-fsanitize=address,undefined','-I'+str(out)]
        for name,path in [('real-validator',source/'src/compiler/nir/nir_validate.c'),('host-validator',helper)]:
            obj=out/(name+'.o');run([*flags,'-c',path,'-o',obj],name+'-compile');validation_objects.append(str(obj))
    observations=[]
    candidate='early-local-ssa-callee' if args.callee else 'early-local-ssa'
    if args.straightline:candidate+='-straightline'
    for variant,data in [('baseline',actual),(candidate,new)]:
        extra=[]
        if args.validate:
            data=data.replace(b'   NIR_PASS(_, shader, nir_lower_returns);\n',b'   NIR_PASS(_, shader, nir_lower_returns);\n   lilac_host_validate_stage(shader,"before-inline");\n')
            anchor=b'   NIR_PASS(_, shader, nir_opt_deref);\n\n   /* We set func->is_entrypoint'
            assert data.count(anchor)==1
            data=data.replace(anchor,b'   NIR_PASS(_, shader, nir_opt_deref);\n   lilac_host_validate_stage(shader,"after-inline");\n\n   /* We set func->is_entrypoint')
            extra=['-include',validation_api]
        folder=out/variant;folder.mkdir();c=folder/'gl_nir_linker.c';c.write_bytes(data)
        obj=folder/'gl_nir_linker.o';run([*compile_flags('gl_nir_linker.c'),*extra,'-c',c,'-o',obj],variant+'-compile')
        library=folder/'libOSMesa.so.8.0.0'
        command=list(base_link);command[command.index('-o')+1]=str(library)
        # All public gl_nir_linker symbols are supplied by the override before
        # libglsl.a. Its original member is not extracted; no archive changes.
        at=command.index('src/compiler/glsl/libglsl.a');command.insert(at,str(obj))
        if validation_objects:
            command[1:1]=['-fsanitize=address,undefined',*validation_objects]
        callee_sources={}
        if args.callee and variant!='baseline':
            functions=source/'src/compiler/nir/nir_functions.c'
            assert sha(functions)=='be2167eda3bcb39dc62762fbc0d7b016e6b59798f33c8d20775b48c50e9dbba1'
            lower=source/'src/compiler/nir/nir_lower_vars_to_ssa.c'
            body=functions.read_bytes();anchor=b'   _mesa_set_add(inlined, impl);\n'
            assert body.count(anchor)==1
            body=body.replace(anchor,b'   bool local_ssa = nir_opt_deref_impl(impl);\n   local_ssa |= nir_lower_vars_to_ssa_impl(impl);\n   if (local_ssa) nir_index_ssa_defs(impl);\n   progress |= local_ssa;\n\n'+anchor)
            if args.straightline:
                anchor=b'   bool nest_if = function_ends_in_jump(copy);\n';assert body.count(anchor)==1
                body=body.replace(anchor,b'   if (exec_list_is_singular(&copy->body) && !function_ends_in_jump(copy)) {\n      nir_block *block=nir_start_block(copy);\n      nir_instr *head=nir_block_first_instr(block);\n      if (!head || head->type!=nir_instr_type_phi) {\n         nir_foreach_instr_safe(instr,block) {\n            nir_instr_remove(instr);\n            nir_builder_instr_insert(b,instr);\n         }\n         return;\n      }\n   }\n\n'+anchor)
            lower_body=lower.read_bytes();declaration=b'static bool\nnir_lower_vars_to_ssa_impl(nir_function_impl *impl)'
            assert lower_body.count(declaration)==1
            lower_body=lower_body.replace(declaration,b'bool\nnir_lower_vars_to_ssa_impl(nir_function_impl *impl)')
            api=folder/'LocalSSAAPI.h';api.write_bytes(b'#include "nir.h"\nbool nir_lower_vars_to_ssa_impl(nir_function_impl *impl);\n')
            objects=[]
            for name,content in [('nir_functions.c',body),('nir_lower_vars_to_ssa.c',lower_body)]:
                path=folder/name;path.write_bytes(content);object_path=folder/(name+'.o')
                run([*compile_flags(name),'-include',api,'-c',path,'-o',object_path],variant+'-'+name+'-compile')
                objects.append(str(object_path));callee_sources[name]=sha(path)
            at=command.index('src/compiler/nir/libnir.a');command[at:at]=objects
        run(command,variant+'-link');(folder/'libOSMesa.so.8').symlink_to(library.name)
        executable=folder/'probe'
        probe=ROOT/'tools/build/probe_original_render_link.c'
        run(['cc','-O1','-g','-fsanitize=address,undefined','-DSWEET_LINK_OSMESA','-I'+str(source/'include'),probe,library,'-o',executable],variant+'-probe-compile')
        env=dict(os.environ,GALLIUM_DRIVER='llvmpipe',MESA_SHADER_CACHE_DISABLE='true',ASAN_OPTIONS='detect_leaks=0',LD_LIBRARY_PATH=str(folder)+':'+str(build/'src/mapi/shared-glapi'))
        result=run([executable,vertex,fragment],variant+'-probe',cwd=ROOT,environment=env,require_success=False)
        text=result.stdout.decode(errors='replace');end=re.search(r'LINK_PROBE end stage=link wall_ns=(\d+) cpu_ns=(\d+) status=(\d+)',text)
        observations.append(dict(variant=variant,source_sha256=sha(c),callee_source_sha256=callee_sources,object_sha256=sha(obj),library_sha256=sha(library),process_returncode=result.returncode,
                                 link_return_observed=bool(end),link=dict(wall_ns=int(end[1]),cpu_ns=int(end[2]),status=int(end[3])) if end else None,
                                 NIR_validation_checks=result.stderr.count(b'HOST_NIR_VALIDATED'),
                                 original_host_LLVM14_teardown_failure_seen=b'lp_passmgr_dispose' in result.stderr,full_host_process_passed=result.returncode==0))
    assert before=={str(p):sha(p) for p in protected}
    report=dict(pinned_Mesa='24.3.4',pinned_commit='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de',immutable_input_hashes=before,commands=commands,observations=observations,
                source_files={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__),ROOT/'tools/build/probe_original_render_link.c')},
                exact_current_observed_shader_inputs=True,compiler_flags_already_O3_NDEBUG=True,callee_optimization=args.callee,straightline_optimization=args.straightline,actual_debug_NIR_validator=args.validate,production_not_changed=True,device_operations=0,
                link_return_or_speed_does_not_promote_semantics_device_or_UI=True)
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode());print(json.dumps(report))


if __name__=='__main__':main()
