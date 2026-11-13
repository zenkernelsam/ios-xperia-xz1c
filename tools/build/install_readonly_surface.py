"""Apply the v727 read-only backing candidate only after its actual host checks."""
import json,subprocess,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from diagnostic_candidate import ROOT,configuration,audit
config=configuration(727);stage=ROOT/'build/surface-readonly-v727'
proof=json.loads((stage/'host-checks.json').read_bytes())
assert proof['actual_direction_and_shared_mapper_asan_ubsan_passed']
assert proof['patch_sha256']==config['manifest']['patch_sha256']
for name in ('test_direction','test_mapper'):
    assert 'PASS' in (stage/(name+'.log')).read_text()
tree=ROOT/'build/dxb/xnu';patch=ROOT/'xnu/lilac-surface-readonly.patch'
reverse=subprocess.run(['git','apply','--reverse','--check',str(patch)],cwd=tree,capture_output=True)
if reverse.returncode:
    audit(726)
    subprocess.run(['git','apply','--check',str(patch)],cwd=tree,check=True)
    subprocess.run(['git','apply',str(patch)],cwd=tree,check=True)
result=audit(727)
output=ROOT/'build/surface-diag-v727';output.mkdir(exist_ok=True)
(output/'installation.json').write_bytes((json.dumps(result,indent=2)+'\n').encode())
print(json.dumps(result,indent=2))
