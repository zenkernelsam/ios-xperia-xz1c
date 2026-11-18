"""Fetch fixed OSS sources and apply the repository's complete Lilac patches."""
import argparse,hashlib,json,subprocess,tempfile,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2];lock=json.loads((root/'reproducibility/source-lock.json').read_text())
sys.path.insert(0,str(root/'tools'))
from baseline_source_inputs import materialize as materialize_baseline
def command(args,cwd=None):subprocess.run(args,cwd=cwd,check=True)
def checkout(url,commit,path):
    if path.exists():
        actual=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()
        assert actual==commit,'Existing source has a different commit: '+str(path)
        return
    path.parent.mkdir(parents=True,exist_ok=True)
    command(['git','init','-q',str(path)])
    command(['git','-C',str(path),'fetch','--depth','1',url,commit])
    command(['git','-C',str(path),'checkout','--detach','-q','FETCH_HEAD'])
def patch(path,file):
    reverse=subprocess.run(['git','-C',str(path),'apply','--reverse','--check',str(file)],capture_output=True)
    if reverse.returncode==0:return
    command(['git','-C',str(path),'apply','--check',str(file)])
    command(['git','-C',str(path),'apply',str(file)])

def mesa_patch(path,file):
    # Retained checkouts can contain a fixed preceding complete patch. Verify
    # that exact prefix and only its missing tail before applying anything.
    data=file.read_bytes()
    assert hashlib.sha256(data).hexdigest()==lock['mesa']['patch_sha256']
    reverse=subprocess.run(['git','-C',str(path),'apply','--reverse','--check',str(file)],capture_output=True)
    if reverse.returncode==0:return
    predecessors=(('src/mesa/state_tracker/st_extensions.c','program_binary_identity_patch_sha256'),
                  ('src/mesa/state_tracker/st_shader_cache.c','program_binary_previous_patch_sha256'))
    for name,key in predecessors:
        anchor=('diff --git a/'+name+' ').encode()
        if anchor not in data or not lock['mesa'].get(key):continue
        previous,delta=data.split(anchor,1);delta=anchor+delta
        assert hashlib.sha256(previous).hexdigest()==lock['mesa'][key]
        out=Path(tempfile.mkdtemp(prefix='lilac-mesa-patch-'));old=out/'previous.patch';new=out/'driver-id.patch'
        old.write_bytes(previous);new.write_bytes(delta)
        old_applied=subprocess.run(['git','-C',str(path),'apply','--reverse','--check',str(old)],capture_output=True)
        delta_ready=subprocess.run(['git','-C',str(path),'apply','--check',str(new)],capture_output=True)
        if old_applied.returncode==0 and delta_ready.returncode==0:
            command(['git','-C',str(path),'apply',str(new)])
            command(['git','-C',str(path),'apply','--reverse','--check',str(file)]);return
    patch(path,file)
p=argparse.ArgumentParser();p.add_argument('component',choices=['mesa','kernel']);a=p.parse_args()
if a.component=='mesa':
    target=root/'deps/mesa';checkout(lock['mesa']['upstream'],lock['mesa']['base'],target);mesa_patch(target,root/'gpu/mesa-lilac.patch')
else:
    target=root/'build/dxb'
    checkout(lock['builder']['upstream'],'663906528d1cd5ea8fde7de6f16de73db9511549',target)
    full=(root/'tools/build/dxb-local.patch').read_bytes()
    script=next(b'diff --git '+part for part in full.split(b'diff --git ') if part.startswith(b'a/build.sh b/build.sh'))
    one=root/'build/dxb-build-script.patch';one.write_bytes(script)
    if 'LILAC_RECONSTRUCTED_SOURCE' not in (target/'build.sh').read_text():
        patch(target,one)
    checkout('https://github.com/apple-oss-distributions/xnu.git',lock['xnu']['base_commit'],target/'xnu')
    patch(target/'xnu',root/'xnu/lilac-current.patch')
    materialize_baseline(root,target/'xnu')
    for name,row in lock['builder_dependencies'].items():
        checkout('https://github.com/apple-oss-distributions/'+name+'.git',row['commit'],target/name)
print('PINNED_SOURCES_PREPARED: '+str(target))
