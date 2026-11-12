"""Reconstruct exported source patches in fresh Git checkouts; never delete old trees."""
import hashlib,json,os,re,subprocess,time
from pathlib import Path
root=Path(__file__).resolve().parents[2];locks=json.loads((root/'reproducibility/source-lock.json').read_text())
build=root/'build'/('source-verification-'+time.strftime('%Y%m%d-%H%M%S'));assert not build.exists();build.mkdir(parents=True)
def git(path,args):
    r=subprocess.run(['git','-C',str(path),*args],capture_output=True,text=True)
    assert r.returncode==0,r.stdout+r.stderr
    return r.stdout
report={}
for name,repo,base,patch in [
    ('xnu',root/'dxb/xnu',locks['xnu']['base_commit'],root/'xnu/lilac-current.patch'),
    ('mesa',root/'research/mesa',locks['mesa']['base'],root/'gpu/mesa-lilac.patch')]:
    target=build/name
    r=subprocess.run(['git','clone','--shared','--no-checkout',str(repo),str(target)],capture_output=True,text=True);assert r.returncode==0,r.stderr
    git(target,['checkout','--detach',base])
    git(target,['apply','--check',str(patch)]);git(target,['apply',str(patch)])
    if name=='xnu':
        checked={}
        for path,digest in locks['audited_xnu_files'].items():
            actual=hashlib.sha256((target/path).read_bytes()).hexdigest();assert actual==digest,path
            checked[path]=actual
        paths=set(git(repo,['diff','--name-only',base]).splitlines())|set(locks['xnu']['extra_sources'])
        paths={p for p in paths if not re.search(r'(\.orig|\.rej|\.bak[^/]*|\.DS_Store)$',p)}
    else:paths=set(git(repo,['diff','--name-only','HEAD']).splitlines())
    for path in paths:
        actual=repo/path;rebuilt=target/path
        assert actual.exists()==rebuilt.exists(),path
        if actual.is_file():assert actual.read_bytes()==rebuilt.read_bytes(),path
    report[name]={'base':base,'changed_paths_exact':len(paths),'fresh_patch_apply_passed':True,
                  'audited_live_source_hashes':len(checked) if name=='xnu' else 0}
out=root/'reproducibility/source-reconstruction.json';out.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
