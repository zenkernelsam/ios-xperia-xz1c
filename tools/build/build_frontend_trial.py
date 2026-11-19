"""Compile the current frontend against audited, unchanged existing Mesa archives."""
import argparse,hashlib,json,os,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--output',required=True);p.add_argument('--mesa-build');a=p.parse_args()
out=Path(a.output).absolute();assert out.is_relative_to(root/'build') and not out.exists()
out.mkdir(parents=True)
source=root/'gpu/sweetgl';mesa=root/'deps/mesa';build=Path(a.mesa_build).absolute() if a.mesa_build else root/'build/mesa-ios'
assert build.is_relative_to(root/'build') and (build/'build.ninja').exists()
lock=json.loads((root/'reproducibility/source-lock.json').read_bytes())
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
sources={str(f.relative_to(source)):sha(f) for f in source.rglob('*') if f.is_file() and not any(p in ('out','__pycache__') for p in f.relative_to(source).parts)}
for path,wanted in lock['sweetgl']['files'].items():
    if path in ('mesa-setup.sh','mesa.patch'):continue
    assert sources[path]==(lock['sweetgl']['build_script_updated_for_canonical_output'] if path=='build.sh' else wanted),path
archives=re.search(r'LIBS="([^"]+)"',(source/'build.sh').read_text()).group(1).split()
archives+=['src/mapi/shared-glapi/libglapi.0.dylib']
dependencies={path:sha(build/path) for path in archives}
env=dict(os.environ,SWEETGL_MESA_SOURCE=str(mesa),SWEETGL_MESA_BUILD=str(build),SWEETGL_OUTPUT=str(out))
with (out/'build.log').open('wb') as log:
    result=subprocess.run(['bash',str(source/'build.sh')],env=env,stdout=log,stderr=subprocess.STDOUT)
(out/'build.rc').write_text(str(result.returncode)+'\n')
assert result.returncode==0,(out/'build.log').read_text(errors='replace')[-3000:]
for path,digest in sources.items():assert sha(source/path)==digest,path
for path,digest in dependencies.items():assert sha(build/path)==digest,path
binding={'source_files':sources,'mesa_dependencies':dependencies,'mesa_base':lock['mesa']['base'],
         'mesa_patch_sha256':lock['mesa']['patch_sha256'],'only_frontend_recompiled':True,
         'library_sha256':sha(out/'sweetgl.dylib'),'device_operations':0}
binding['XR_clear_decode_elision']=True
binding['Clear_shader_context_cache']=True
binding['render_target_observation_revision']=1
binding['mesa_build_directory']=build.relative_to(root).as_posix()
if (build/'ir3-variant-cache-inputs.json').exists():
    inputs=json.loads((build/'ir3-variant-cache-inputs.json').read_bytes())
    assert inputs==lock['mesa']['IR3_variant_cache']
    binding['IR3_variant_cache']=inputs
    binding['actual_IR3_variant_cache_GPU_acceptance_pending']=True
if (out/'ProgramCacheBuildIdentity.json').exists():
    binding['program_cache_build_identity']=json.loads((out/'ProgramCacheBuildIdentity.json').read_bytes())
(out/'build-binding.json').write_bytes((json.dumps(binding,indent=2)+'\n').encode())
print(json.dumps({'output':str(out),'library_sha256':binding['library_sha256'],'source_files':len(sources),'existing_mesa_inputs':len(dependencies)}))
