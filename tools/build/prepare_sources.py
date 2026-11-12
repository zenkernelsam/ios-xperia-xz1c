"""Fetch fixed OSS sources and apply the repository's complete Lilac patches."""
import argparse,json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[2];lock=json.loads((root/'reproducibility/source-lock.json').read_text())
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
p=argparse.ArgumentParser();p.add_argument('component',choices=['mesa','kernel']);a=p.parse_args()
if a.component=='mesa':
    target=root/'deps/mesa';checkout(lock['mesa']['upstream'],lock['mesa']['base'],target);patch(target,root/'gpu/mesa-lilac.patch')
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
    for name,row in lock['builder_dependencies'].items():
        checkout('https://github.com/apple-oss-distributions/'+name+'.git',row['commit'],target/name)
print('PINNED_SOURCES_PREPARED: '+str(target))
