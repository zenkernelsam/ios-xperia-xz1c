"""Create reproducible host-check inputs from an audited tree and published deltas."""
import argparse,json,shutil,subprocess,sys,time
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT,configuration,audit,sha
p=argparse.ArgumentParser();p.add_argument('--version',type=int,choices=(727,728),required=True);a=p.parse_args()
config=configuration(a.version);base=a.version-1;old=configuration(base)
tree=ROOT/'build/dxb/xnu'
try:audit(a.version);installed=True
except AssertionError:audit(base);installed=False
stage=ROOT/f'build/surface-readonly-v{a.version}';stage.mkdir(exist_ok=True)
work=stage/('materialized-'+time.strftime('%Y%m%d-%H%M%S'));assert not work.exists();work.mkdir()
paths=config['manifest']['changed_sources'];draft=work/'tree';draft.mkdir()
for path in paths:
    file=draft/path;file.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(tree/path,file)
patch=ROOT/'xnu'/('lilac-surface-readonly.patch' if a.version==727 else 'lilac-surface-region64.patch')
if installed:
    subprocess.run(['git','apply','--reverse',str(patch)],cwd=draft,check=True)
for path in paths:assert sha((draft/path).read_bytes())==old['source_hashes'][path],path
before=stage/'before';before.mkdir(exist_ok=True)
for path in paths:
    target=before/Path(path).name
    if target.exists():assert target.read_bytes()==(draft/path).read_bytes()
    else:shutil.copy2(draft/path,target)
subprocess.run(['git','apply',str(patch)],cwd=draft,check=True)
candidate=stage/'candidate';candidate.mkdir(exist_ok=True)
for path in paths:
    source=draft/path;assert sha(source.read_bytes())==config['source_hashes'][path]
    target=candidate/Path(path).name
    if target.exists():assert target.read_bytes()==source.read_bytes()
    else:shutil.copy2(source,target)
print(json.dumps({'version':a.version,'predecessor':base,'installed':installed,'host_inputs_materialized':True,
                  'work':str(work),'device_operations':0}))
