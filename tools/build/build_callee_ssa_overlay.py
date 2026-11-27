"""Build isolated ARM64 Mesa compiler archives over the audited iOS build.

The four-file OSS patch and rebuilt glsl/nir archives live in a new build
directory. Original deps, archives, GPU backend and kernel remain unchanged.
The overlay is read-only input to build_frontend_trial, never a ninja target.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]


def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--mesa-build',required=True);parser.add_argument('--output',required=True);args=parser.parse_args()
    base,out=(Path(p).resolve() for p in (args.mesa_build,args.output));mesa=ROOT/'deps/mesa'
    assert sys.platform=='darwin' and base.is_relative_to(ROOT/'build') and out.is_relative_to(ROOT/'build') and not out.exists()
    patch=ROOT/'gpu/nir-callee-local-ssa.patch'
    reference=ROOT/'reproducibility/nir-callee-local-ssa-source-binding-20261128.json'
    binding=json.loads(reference.read_bytes());assert binding['Mesa_commit']=='769e51468b49b2a42f0a0eaf71cf9eed5ff4e5de'
    assert subprocess.check_output(['git','rev-parse','HEAD'],cwd=mesa,text=True).strip()==binding['Mesa_commit']
    machines=json.loads((base/'meson-info/intro-machines.json').read_bytes())
    assert machines['host']['cpu_family']=='aarch64' and machines['host']['subsystem']=='ios'
    commands=json.loads((base/'compile_commands.json').read_bytes())
    script=(ROOT/'gpu/sweetgl/build.sh').read_text()
    libraries=re.search(r'LIBS="([^"]+)"',script).group(1).split()+['src/mapi/shared-glapi/libglapi.0.dylib']
    before_archives={p:sha(base/p) for p in libraries}
    before_sources={p:sha(mesa/p) for p in binding['sources']}
    assert all(before_sources[p]==v['before_sha256'] for p,v in binding['sources'].items())
    out.mkdir(parents=True);sources=out/'sources';sources.mkdir()
    for relative in binding['sources']:
        target=sources/relative;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(mesa/relative,target)
    logs=[]
    def run(command,name,cwd=base):
        result=subprocess.run(list(map(str,command)),cwd=cwd,capture_output=True,timeout=90)
        (out/(name+'.log')).write_bytes(result.stdout+result.stderr)
        logs.append(dict(name=name,command=list(map(str,command)),cwd=str(cwd),returncode=result.returncode))
        assert result.returncode==0,result.stderr.decode(errors='replace')
        return result
    directory='--directory='+sources.relative_to(ROOT).as_posix()
    run(['git','apply','--check',directory,patch],'patch-check',ROOT)
    run(['git','apply',directory,patch],'patch-apply',ROOT)
    assert all(sha(sources/p)==v['after_sha256'] for p,v in binding['sources'].items())
    objects=out/'objects';objects.mkdir();compiled={}
    for relative in binding['sources']:
        if not relative.endswith('.c'):continue
        row=next(row for row in commands if row['file'].endswith('/'+relative.split('/')[-1]))
        flags=shlex.split(row['command']);assert '-MD' in flags;flags=flags[:flags.index('-MD')]
        assert '-arch' in flags and flags[flags.index('-arch')+1]=='arm64' and '-O3' in flags and '-DNDEBUG' in flags
        target=objects/(Path(relative).name+'.o')
        run([*flags,'-include',sources/'src/compiler/nir/nir.h','-c',sources/relative,'-o',target],Path(relative).stem+'-compile')
        architecture=run(['xcrun','lipo','-info',target],Path(relative).stem+'-architecture').stdout.decode()
        assert 'arm64' in architecture
        compiled[relative]=dict(object=str(target.relative_to(ROOT)),sha256=sha(target),architecture='arm64')
    replacements={
        'src/compiler/glsl/libglsl.a':[objects/'gl_nir_linker.c.o'],
        'src/compiler/nir/libnir.a':[objects/'nir_functions.c.o',objects/'nir_lower_vars_to_ssa.c.o'],
    }
    overlay=out/'mesa-overlay';overlay.mkdir()
    # Recurse only through ancestors of replacements. All other directories
    # and files are immutable symlink inputs, never written by the builder.
    def populate(original,target,relative=''):
        for item in original.iterdir():
            name=(relative+'/'+item.name).lstrip('/')
            dest=target/item.name
            if name in replacements:continue
            if item.is_dir() and any(key.startswith(name+'/') for key in replacements):
                dest.mkdir();populate(item,dest,name)
            else:dest.symlink_to(item,target_is_directory=item.is_dir())
    populate(base,overlay)
    updated={}
    for relative,members in replacements.items():
        archive=overlay/relative;archive.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(base/relative,archive)
        old=run(['xcrun','ar','t',base/relative],Path(relative).stem+'-original-members').stdout.decode().splitlines()
        assert all(old.count(p.name)==1 for p in members)
        run(['xcrun','ar','r',archive,*members],Path(relative).stem+'-replace')
        actual=run(['xcrun','ar','t',archive],Path(relative).stem+'-new-members').stdout.decode().splitlines()
        assert actual==old,'Archive member order or multiplicity changed'
        updated[relative]=dict(before_sha256=before_archives[relative],after_sha256=sha(archive),replaced_members=[p.name for p in members])
    symbols=run(['xcrun','nm','-g',overlay/'src/compiler/nir/libnir.a'],'nir-symbols').stdout.decode()
    assert len(re.findall(r'\bT _nir_lower_vars_to_ssa_impl$',symbols,re.M))==1
    assert before_archives=={p:sha(base/p) for p in libraries}
    assert before_sources=={p:sha(mesa/p) for p in binding['sources']}
    overlay_archives={p:sha(overlay/p) for p in libraries}
    assert {p for p in libraries if before_archives[p]!=overlay_archives[p]}==set(replacements)
    report=dict(revision=1,Mesa_version='24.3.4',Mesa_commit=binding['Mesa_commit'],
                patch='gpu/nir-callee-local-ssa.patch',patch_sha256=sha(patch),
                source_binding_sha256=sha(reference),sources=binding['sources'],compiled_objects=compiled,
                original_mesa_build=base.relative_to(ROOT).as_posix(),original_archives=before_archives,
                overlay_archives=overlay_archives,updated_archives=updated,commands=logs,
                original_sources_archives_and_kernel_unchanged=True,link_input_overlay_only=True,
                build_frontend_and_new_device_acceptance_required=True,device_operations=0,
                builder_sha256=sha(Path(__file__)))
    (overlay/'nir-callee-local-ssa-inputs.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    (out/'manifest.json').write_bytes((json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps(dict(overlay=str(overlay),updated_archives=updated,arm64_objects=3,device_operations=0)))


if __name__=='__main__':main()
